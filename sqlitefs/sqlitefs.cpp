#include "sqlitefs/sqlitefs.h"
#include <cstring>
#include <mutex>
#include <optional>
#include <SQLiteCpp/SQLiteCpp.h>
#include "sqlqueries.h"
#include "utils.h"


#ifdef MZ_ENABLE
#include "compression.h"

#ifdef HAVE_BZIP
#include <mz_strm_bzip.h>
#endif

#ifdef HAVE_LZMA
#include <mz_strm_lzma.h>
#endif

#ifdef HAVE_ZLIB
#include <mz_strm_zlib.h>
#endif

#ifdef HAVE_ZSTD
#include <mz_strm_zstd.h>
#endif

#endif // MZ_ENABLE


#define ROOT 0

using namespace std::literals;

namespace
{
struct SecureString final : public std::string {
    using std::string::basic_string;
    ~SecureString() { std::memset(data(), 0, size()); };
};
} // namespace

struct SQLiteFS::Impl {
    Impl(std::string path, std::string_view key)
      : m_db_path(std::move(path)), m_db(m_db_path, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE) {
        if (!key.empty()) {
            SecureString secure{key};
            if (SQLite::Database::isUnencrypted(m_db_path)) {
                m_db.rekey(secure);
            } else {
                m_db.key(secure);
            }
        }

        m_db.exec("PRAGMA foreign_keys = ON");
        SQLite::Transaction transaction(m_db);
        for (const auto& q : INIT_DB) {
            m_db.exec(q);
        }
        transaction.commit();
    }

    bool mkdir(const std::string& full_path) {
        SQLITE_SCOPED_PROFILER;

        std::lock_guard lock(m_mutex);

        const auto& [path_id, name] = splitPathAndName(full_path);
        if (!path_id) {
            m_last_error = "Can't find path";
            return false;
        }

        return exec(MKDIR, *path_id, name);
    }

    bool cd(const std::string& path) {
        SQLITE_SCOPED_PROFILER;

        std::lock_guard lock(m_mutex);

        auto path_id = resolve(path);
        if (!path_id) {
            return false;
        }

        auto n = node(*path_id);
        if (n && n->attributes != SQLiteFSNode::Attributes::FILE) {
            m_cwd = n->id;
            return true;
        }

        m_last_error = "Can't find path";
        return false;
    }

    bool rm(const std::string& path) {
        SQLITE_SCOPED_PROFILER;

        if (path == "/") {
            return false;
        }

        std::lock_guard lock(m_mutex);

        auto path_id = resolve(path);
        if (!path_id) {
            return false;
        }
        auto result = exec(RM, *path_id);

        // if folder in current path was removed
        if (!node(m_cwd)) {
            m_cwd = ROOT;
        }

        return result;
    }

    std::string pwd() const {
        SQLITE_SCOPED_PROFILER;

        std::lock_guard lock(m_mutex);

        auto query = select(PWD, m_cwd);
        return query.executeStep() ? query.getColumn(0).getString() : "";
    }

    std::vector<SQLiteFSNode> ls(const std::string& path) const {
        SQLITE_SCOPED_PROFILER;

        std::lock_guard lock(m_mutex);

        auto current_node = node(path + "/");
        if (!current_node) {
            return {};
        }

        std::vector<SQLiteFSNode> content;

        if (current_node->attributes != SQLiteFSNode::Attributes::FILE) {
            auto query = select(LS, current_node->id);
            while (auto child_node = node(query)) {
                content.emplace_back(std::move(*child_node));
            }
            return content;
        }

        return {*current_node};
    }

    bool write(const std::string& full_path, DataInput data, const std::string& alg) {
        SQLITE_SCOPED_PROFILER;

        auto data_modified = internalCall(alg, data, m_save_funcs);

        std::lock_guard lock(m_mutex);

        const auto& [path_id, name] = splitPathAndName(full_path);
        if (!path_id || name.empty()) {
            m_last_error = "Can't find path";
            return false;
        }


        bool                success = true;
        SQLite::Transaction transaction(m_db);

        success &= exec(TOUCH,
                        *path_id,
                        name,
                        static_cast<std::int64_t>(data_modified.size()),
                        static_cast<std::int64_t>(data.size()),
                        alg);

        auto new_node = node(*path_id, name);
        success &= new_node && saveBlob(new_node->id, data_modified);

        if (success) {
            transaction.commit();
        } else {
            m_last_error = "Internal error: Can't write data";
            transaction.rollback();
        }

        return success;
    }

    DataOutput read(const std::string& full_path) const {
        SQLITE_SCOPED_PROFILER;

        std::lock_guard lock(m_mutex);

        auto id = resolve(full_path);
        if (!id) {
            return {};
        }

        auto current_node = node(*id);
        if (current_node && current_node->attributes == SQLiteFSNode::Attributes::FILE) {
            auto data_query = select(GET_FILE_DATA, *id);
            if (!data_query.executeStep()) {
                assert(false && "internal error: DB is broken. No data for file node");
                return {};
            }

            auto data = data_query.getColumn(0).getString();
            auto view = std::span{reinterpret_cast<const std::uint8_t*>(data.data()), data.size()};

            const auto& raw_data = internalCall(current_node->compression, view, m_load_funcs);
            if (static_cast<std::size_t>(current_node->size_raw) != raw_data.size()) {
                m_last_error = "File size doesn't mach.\nFS meta - "s + std::to_string(current_node->size_raw) +
                               ", File - " + std::to_string(raw_data.size());
            }
            return raw_data;
        }

        m_last_error = "Can't read folder data";
        return {};
    }

    bool mv(const std::string& from, const std::string& to) {
        SQLITE_SCOPED_PROFILER;

        std::lock_guard lock(m_mutex);

        auto [target_path_id, target_name] = splitPathAndName(to);
        if (!target_path_id) {
            return false;
        }

        auto source = node(from);
        if (!source) {
            return false;
        }

        if (target_name.empty()) {
            target_name = source->name;
        }


        if (auto target = node(*target_path_id, target_name);
            target && target->attributes == SQLiteFSNode::Attributes::FILE) {
            m_last_error = "The target cannot be an existing file";
            return false;
        }


        auto                success = true;
        SQLite::Transaction transaction(m_db);

        success &= exec(SET_PARENT_ID, *target_path_id, source->id);
        if (source->name != target_name) {
            success &= exec(SET_NAME, target_name, source->id);
        }

        if (success) {
            transaction.commit();
        } else {
            m_last_error = "Internal error: can't move node";
            transaction.rollback();
        }

        return success;
    }

    bool cp(const std::string& from, const std::string& to) {
        SQLITE_SCOPED_PROFILER;

        std::lock_guard lock(m_mutex);

        auto [target_path_id, target_name] = splitPathAndName(to);
        if (!target_path_id) {
            return false;
        }

        auto source = node(from);
        if (!source) {
            return false;
        }

        if (target_name.empty()) {
            target_name = source->name;
        }


        if (auto target = node(*target_path_id, target_name);
            target && target->attributes == SQLiteFSNode::Attributes::FILE) {
            m_last_error = "The target cannot be an existing file";
            return false;
        }


        bool                success = true;
        SQLite::Transaction transaction(m_db);

        success &= exec(COPY_FILE_FS, *target_path_id, target_name, source->id);
        success &= exec(COPY_FILE_RAW, source->id);

        if (success) {
            transaction.commit();
        } else {
            m_last_error = "Internal error: can't copy node";
            transaction.rollback();
        }

        return success;
    }

    void vacuum() {
        SQLITE_SCOPED_PROFILER;
        exec("VACUUM");
    }

    std::string error() const {
        std::string temp;
        {
            std::lock_guard lock(m_mutex);
            temp.swap(m_last_error);
        }
        return temp;
    };

    const std::string& path() const noexcept { return m_db_path; }

    void registerSaveFunc(const std::string& name, const ConvertFunc& func) {
        SQLITE_SCOPED_PROFILER;
        assert(!m_save_funcs.contains(name));
        m_save_funcs.try_emplace(name, func);
    }
    void registerLoadFunc(const std::string& name, const ConvertFunc& func) {
        SQLITE_SCOPED_PROFILER;
        assert(!m_load_funcs.contains(name));
        m_load_funcs.try_emplace(name, func);
    }

    DataOutput callSaveFunc(const std::string& name, DataInput data) {
        SQLITE_SCOPED_PROFILER;
        return internalCall(name, data, m_save_funcs);
    }
    DataOutput callLoadFunc(const std::string& name, DataInput data) {
        SQLITE_SCOPED_PROFILER;
        return internalCall(name, data, m_load_funcs);
    }

    void rawCall(const std::function<void(SQLite::Database*)>& callback) {
        SQLITE_SCOPED_PROFILER;
        std::lock_guard lock(m_mutex);
        callback(&m_db);
    }

private:
    static DataOutput internalCall(const std::string& name, DataInput data, const ConvertFuncsMap& map) {
        auto it = map.find(name);
        if (it != map.end()) {
            return it->second(data);
        }
        assert(false && "Function doesn't exist");
        return {};
    }

    template<typename... Args>
    int exec(const std::string& query_string, Args&&... args) {
        SQLITE_SCOPED_PROFILER;

        try {
            SQLite::Statement query{m_db, query_string};

            [&]<size_t... Is>(std::index_sequence<Is...>) {
                (query.bind(Is + 1, std::forward<Args>(args)), ...);
            }(std::make_index_sequence<sizeof...(Args)>{});
            return query.exec();
        } catch (std::exception& e) { m_last_error = "SQL Error: "s + e.what(); }
        return 0;
    }

    template<typename... Args>
    SQLite::Statement select(const std::string& query_string, Args&&... args) const {
        SQLITE_SCOPED_PROFILER;

        SQLite::Statement query{m_db, query_string};

        [&]<size_t... Is>(std::index_sequence<Is...>) {
            (query.bind(Is + 1, std::forward<Args>(args)), ...);
        }(std::make_index_sequence<sizeof...(Args)>{});
        return query;
    }

    bool saveBlob(std::uint32_t id, DataInput data) {
        SQLITE_SCOPED_PROFILER;

        try {
            SQLite::Statement query{m_db, SET_FILE_DATA};
            query.bind(1, id);
            query.bind(2, data.data(), data.size());
            return query.exec();
        } catch (std::exception& e) { m_last_error = "SQL Error: "s + e.what(); }
        return false;
    }


    std::optional<SQLiteFSNode> node(const std::string& path) const {
        SQLITE_SCOPED_PROFILER;

        const auto& [path_id, name] = splitPathAndName(path);
        if (!path_id) {
            m_last_error = "Can't find path";
            return std::nullopt;
        }
        return name.empty() ? node(*path_id) : node(*path_id, name);
    }

    std::optional<SQLiteFSNode> node(std::uint32_t id) const {
        SQLITE_SCOPED_PROFILER;

        auto query = select(GET_NODE_BY_ID, id);
        return node(query);
    }


    std::optional<SQLiteFSNode> node(std::uint32_t path_id, const std::string& name) const {
        SQLITE_SCOPED_PROFILER;

        auto query = select(GET_NODE, path_id, name);
        return node(query);
    }

    std::optional<SQLiteFSNode> node(SQLite::Statement& query) const {
        SQLITE_SCOPED_PROFILER;

        if (query.executeStep()) {
            SQLiteFSNode out;
            out.id          = query.getColumn(0).getUInt();
            out.parent_id   = query.getColumn(1).getUInt();
            out.name        = query.getColumn(2).getString();
            out.attributes  = static_cast<SQLiteFSNode::Attributes>(query.getColumn(3).getInt());
            out.size        = query.getColumn(4).getInt64();
            out.size_raw    = query.getColumn(5).getInt64();
            out.compression = query.getColumn(6).getString();

            return out;
        }

        m_last_error = "Can't find node";
        return std::nullopt;
    }

    std::optional<std::uint32_t> resolve(const std::string& path) const {
        SQLITE_SCOPED_PROFILER;

        if (path.empty()) {
            return m_cwd;
        }

        if (path == "/") {
            return ROOT;
        }

        std::uint32_t id    = path.starts_with('/') ? ROOT : m_cwd;
        auto          names = split(path, '/');

        for (const auto& name : names) {
            if (name == ".") {
                continue;
            }

            auto current_node = node(id);
            if (!current_node) {
                return std::nullopt;
            }

            if (name == "..") {
                id = current_node->parent_id;
                continue;
            }

            if (auto n = node(id, name); n) {
                id = n->id;
                continue;
            }

            m_last_error = "Can't find target path";
            return std::nullopt;
        }
        return id;
    }

    std::pair<std::optional<std::uint32_t>, std::string> splitPathAndName(const std::string& full_path) const {
        SQLITE_SCOPED_PROFILER;

        auto pos = full_path.find_last_of('/');

        if (pos == std::string::npos) {
            return {m_cwd, full_path};
        }

        auto path = full_path.substr(0, pos + 1);
        auto name = full_path.substr(pos + 1);
        return {resolve(path), std::move(name)};
    }

private:
    std::string         m_db_path;
    std::uint32_t       m_cwd = ROOT;
    SQLite::Database    m_db;
    mutable std::string m_last_error;
    SQLITE_LOCABLE_PROFILER(std::mutex, m_mutex);
    SQLITEFS_NO_PROFILER(mutable std::mutex m_mutex);

    ConvertFuncsMap m_save_funcs;
    ConvertFuncsMap m_load_funcs;
};


SQLiteFS::SQLiteFS(std::string path, std::string_view key) : m_impl(std::make_unique<Impl>(std::move(path), key)) {
    SQLiteFS::registerSaveFunc("raw", [](DataInput data) { return DataOutput{data.begin(), data.end()}; });
    SQLiteFS::registerLoadFunc("raw", [](DataInput data) { return DataOutput{data.begin(), data.end()}; });

#ifdef HAVE_BZIP
    SQLiteFS::registerSaveFunc("bzip", [](DataInput data) { return minizipCompress(data, mz_stream_bzip_create); });
    SQLiteFS::registerLoadFunc("bzip", [](DataInput data) { return minizipDecompress(data, mz_stream_bzip_create); });
#endif

#ifdef HAVE_LZMA
    SQLiteFS::registerSaveFunc("lzma", [](DataInput data) { return minizipCompress(data, mz_stream_lzma_create); });
    SQLiteFS::registerLoadFunc("lzma", [](DataInput data) { return minizipDecompress(data, mz_stream_lzma_create); });
#endif

#ifdef HAVE_ZLIB
    SQLiteFS::registerSaveFunc("zlib", [](DataInput data) { return minizipCompress(data, mz_stream_zlib_create); });
    SQLiteFS::registerLoadFunc("zlib", [](DataInput data) { return minizipDecompress(data, mz_stream_zlib_create); });
#endif

#ifdef HAVE_ZSTD
    SQLiteFS::registerSaveFunc("zstd", [](DataInput data) { return minizipCompress(data, mz_stream_zstd_create); });
    SQLiteFS::registerLoadFunc("zstd", [](DataInput data) { return minizipDecompress(data, mz_stream_zstd_create); });
#endif
}


std::string SQLiteFS::pwd() const {
    return m_impl->pwd();
}

std::vector<SQLiteFSNode> SQLiteFS::ls(const std::string& path) const {
    return m_impl->ls(path);
}

bool SQLiteFS::mkdir(const std::string& name) {
    return m_impl->mkdir(name);
}

bool SQLiteFS::cd(const std::string& name) {
    return m_impl->cd(name);
}

bool SQLiteFS::rm(const std::string& name) {
    return m_impl->rm(name);
}

bool SQLiteFS::write(const std::string& name, DataInput data, const std::string& alg) {
    return m_impl->write(name, data, alg);
}

SQLiteFS::DataOutput SQLiteFS::read(const std::string& name) const {
    return m_impl->read(name);
}

bool SQLiteFS::mv(const std::string& from, const std::string& to) {
    return m_impl->mv(from, to);
}

bool SQLiteFS::cp(const std::string& from, const std::string& to) {
    return m_impl->cp(from, to);
}

const std::string& SQLiteFS::path() const noexcept {
    return m_impl->path();
}

void SQLiteFS::vacuum() {
    m_impl->vacuum();
}

std::string SQLiteFS::error() const {
    return m_impl->error();
}

void SQLiteFS::registerSaveFunc(const std::string& name, const ConvertFunc& func) {
    m_impl->registerSaveFunc(name, func);
}

void SQLiteFS::registerLoadFunc(const std::string& name, const ConvertFunc& func) {
    m_impl->registerLoadFunc(name, func);
}

SQLiteFS::DataOutput SQLiteFS::callSaveFunc(const std::string& name, DataInput data) {
    return m_impl->callSaveFunc(name, data);
}

SQLiteFS::DataOutput SQLiteFS::callLoadFunc(const std::string& name, DataInput data) {
    return m_impl->callLoadFunc(name, data);
}

void SQLiteFS::rawCall(const std::function<void(SQLite::Database*)>& callback) {
    m_impl->rawCall(callback);
}

SQLiteFS::~SQLiteFS() {} // NOLINT
