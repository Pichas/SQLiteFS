#include "sqlitefs/sqlitefs.h"
#include <optional>
#include <SQLiteCpp/SQLiteCpp.h>
#include <spdlog/spdlog.h>
#include <spdlog/stopwatch.h>
#include "sqlqueries.h"
#include "utils.h"

#define ROOT 0

struct SQLiteFS::Impl {
    Impl(std::string path) : m_db_path(std::move(path)), m_db(m_db_path, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE) {
        SQLite::Transaction transaction(m_db);
        for (const auto& q : INIT_DB) {
            m_db.exec(q);
        }
        transaction.commit();
    }

    bool mkdir(const std::string& full_path) {
        std::lock_guard lock(m_mutex);

        const auto& [path_id, name] = getPathAndName(full_path);
        if (!path_id) {
            return false;
        }

        return exec(MKDIR, *path_id, name);
    }


    bool cd(const std::string& path) {
        std::lock_guard lock(m_mutex);

        if (auto node = pathResolver(path); node) {
            m_cwd = *node;
            return true;
        }
        return false;
    }

    bool rm(const std::string& full_path) {
        std::lock_guard lock(m_mutex);

        const auto& [path_id, name] = getPathAndName(full_path);
        if (!path_id) {
            return false;
        }

        return exec(RM, *path_id, name);
    }

    std::string pwd() const {
        std::lock_guard lock(m_mutex);

        auto query = select(PWD, m_cwd);
        if (query.executeStep()) {
            return query.getColumn(0).getString();
        }

        return "Folder doesn't exist";
    }

    std::vector<SQLiteFSNode> ls(const std::string& path) const {
        std::lock_guard lock(m_mutex);

        std::vector<SQLiteFSNode> content;

        const auto& [path_id, name] = getPathAndName(path + "/");
        if (!path_id) {
            return content;
        }

        auto query = select(LS, *path_id);
        while (auto node = getNodeData(query)) {
            content.emplace_back(std::move(*node));
        }

        return content;
    }

    bool put(const std::string& full_path, DataInput data, const std::string& alg) {
        auto data_modified = internalCall(alg, data, m_save_funcs);

        std::lock_guard lock(m_mutex);

        const auto& [path_id, name] = getPathAndName(full_path);
        if (!path_id) {
            return false;
        }

        SQLite::Transaction transaction(m_db);
        if (exec(TOUCH,
                 *path_id,
                 name,
                 static_cast<std::int64_t>(data_modified.size()),
                 static_cast<std::int64_t>(data.size()),
                 alg)) {
            auto id = getNodeId(*path_id, name);
            if (id && saveBlob(*id, data_modified)) {
                transaction.commit();
                return true;
            }
        }

        return false;
    }

    DataOutput get(const std::string& full_path) const {
        std::lock_guard lock(m_mutex);

        const auto& [path_id, name] = getPathAndName(full_path);
        if (!path_id) {
            return {};
        }

        auto id = getNodeId(*path_id, name);
        if (!id) {
            return {};
        }

        auto data_query = select(GET_FILE_DATA, *id);
        if (data_query.executeStep()) {
            auto data = data_query.getColumn(0).getString();
            auto view = std::span{reinterpret_cast<const std::uint8_t*>(data.data()), data.size()};

            auto file_info_query = select(GET_INFO, *id);
            auto file            = getNodeData(file_info_query);
            if (!file) {
                spdlog::critical("DB is broken. Can't find file info {}", *id);
                return {};
            }

            const auto& raw_data = internalCall(file->compression, view, m_load_funcs);
            if (static_cast<std::size_t>(file->size_raw) != raw_data.size()) {
                spdlog::error("File size doesn't mach.\nFS meta - {}, File - {}", file->size_raw, raw_data.size());
            }
            return raw_data;
        }
        return {};
    }

    bool mv(const std::string& from, const std::string& to) {
        std::lock_guard lock(m_mutex);

        const auto& [f_path_id, f_name] = getPathAndName(from);
        if (!f_path_id) {
            return false;
        }

        auto f_id = getNodeId(*f_path_id, f_name);
        if (!f_id) {
            return false;
        }

        const auto& [t_path_id, t_name] = getPathAndName(to);
        if (!t_path_id) {
            return false;
        }

        if (!exec(SET_PARENT_ID, *t_path_id, *f_id)) {
            spdlog::error("internal error: can't move file");
            return false;
        }

        if (!exec(SET_NAME, t_name, *f_id)) {
            spdlog::error("internal error: can't move file");
            return false;
        }

        return true;
    }

    bool cp(const std::string& from, const std::string& to) {
        std::lock_guard lock(m_mutex);

        const auto& [f_path_id, f_name] = getPathAndName(from);
        if (!f_path_id) {
            return false;
        }

        auto f_id = getNodeId(*f_path_id, f_name);
        if (!f_id) {
            return false;
        }

        auto file_info_query = select(GET_INFO, *f_id);
        auto file            = getNodeData(file_info_query);
        if (!file->is_file) {
            spdlog::error("you can copy only files and one by one");
            return false;
        }

        const auto& [t_path_id, t_name] = getPathAndName(to);
        if (!t_path_id) {
            return false;
        }

        if (!exec(COPY_FILE_FS, *t_path_id, t_name, *f_id)) {
            spdlog::error("internal error: can't copy file");
            return false;
        }

        auto t_id = getNodeId(*t_path_id, t_name);
        assert(t_id && "internal error: can't copy file");

        if (!exec(COPY_FILE_RAW, *t_id, *f_id)) {
            spdlog::error("internal error: can't copy file");
            return false;
        }

        return true;
    }

    const std::string& path() const noexcept { return m_db_path; }

    void registerSaveFunc(const std::string& name, const ConvertFunc& func) { m_save_funcs.try_emplace(name, func); }
    void registerLoadFunc(const std::string& name, const ConvertFunc& func) { m_load_funcs.try_emplace(name, func); }

    DataOutput callSaveFunc(const std::string& name, DataInput data) { return internalCall(name, data, m_save_funcs); }
    DataOutput callLoadFunc(const std::string& name, DataInput data) { return internalCall(name, data, m_load_funcs); }


private:
    static DataOutput internalCall(const std::string& name, DataInput data, const ConvertFuncsMap& map) {
        auto it = map.find(name);
        if (it != map.end()) {
            return it->second(data);
        }
        spdlog::error("Cannot find {}", name);
        return {};
    }

    template<typename... Args>
    int exec(const std::string& query_string, Args&&... args) {
        try {
            SQLite::Statement query{m_db, query_string};

            [&]<size_t... Is>(std::index_sequence<Is...>) {
                (query.bind(Is + 1, std::forward<Args>(args)), ...);
            }(std::make_index_sequence<sizeof...(Args)>{});
            return query.exec();
        } catch (std::exception& e) { spdlog::critical("SQL Error: {} ", e.what()); }
        return 0;
    }

    template<typename... Args>
    SQLite::Statement select(const std::string& query_string, Args&&... args) const {
        SQLite::Statement query{m_db, query_string};

        [&]<size_t... Is>(std::index_sequence<Is...>) {
            (query.bind(Is + 1, std::forward<Args>(args)), ...);
        }(std::make_index_sequence<sizeof...(Args)>{});
        return query;
    }

    bool saveBlob(std::uint32_t id, DataInput data) {
        try {
            SQLite::Statement query{m_db, SET_FILE_DATA};
            query.bind(1, id);
            query.bind(2, data.data(), data.size());
            return query.exec();
        } catch (std::exception& e) { spdlog::critical("SQL Error: {} ", e.what()); }
        return false;
    }

    static std::optional<SQLiteFSNode> getNodeData(SQLite::Statement& query) {
        if (query.executeStep()) {
            SQLiteFSNode out;
            out.id          = query.getColumn(0).getInt();
            out.parent_id   = query.getColumn(1).getInt();
            out.name        = query.getColumn(2).getString();
            out.size        = query.getColumn(4).getInt();
            out.size_raw    = query.getColumn(5).getInt();
            out.compression = query.getColumn(6).getString();

            auto attrib = query.getColumn(3).getInt();
            out.is_file = attrib & (1 << 0);
            return out;
        }
        return {};
    }

    std::optional<std::uint32_t> getNodeId(std::uint32_t path_id, const std::string& name) const {
        auto query = select(GET_ID, path_id, name);

        if (query.executeStep()) {
            return query.getColumn(0).getInt();
        }
        return {};
    }

    std::optional<std::uint32_t> getParentId(std::uint32_t path_id) const {
        auto query = select(GET_PARENT_ID, path_id);

        if (query.executeStep()) {
            return query.getColumn(0).getInt();
        }
        return {};
    }

    std::optional<std::uint32_t> pathResolver(const std::string& path) const {
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

            if (auto node_id = name == ".." ? getParentId(id) : getNodeId(id, name)) {
                id = *node_id;
                continue;
            }

            return {};
        }
        return id;
    }

    std::pair<std::optional<std::uint32_t>, std::string> getPathAndName(const std::string& full_path) const {
        auto pos = full_path.find_last_of('/');

        if (pos == std::string::npos) {
            return {m_cwd, full_path};
        }

        auto path = full_path.substr(0, pos + 1);
        auto name = full_path.substr(pos + 1);
        return {pathResolver(path), name};
    }

private:
    std::string              m_db_path;
    std::uint32_t            m_cwd = ROOT;
    mutable std::mutex       m_mutex;
    mutable SQLite::Database m_db;

    ConvertFuncsMap m_save_funcs;
    ConvertFuncsMap m_load_funcs;
};


SQLiteFS::SQLiteFS(std::string path) : m_impl(std::make_unique<Impl>(std::move(path))) {
    SQLiteFS::registerSaveFunc("raw", [](DataInput data) { return DataOutput{data.begin(), data.end()}; });
    SQLiteFS::registerLoadFunc("raw", [](DataInput data) { return DataOutput{data.begin(), data.end()}; });
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

bool SQLiteFS::put(const std::string& name, DataInput data, const std::string& alg) {
    return m_impl->put(name, data, alg);
}

SQLiteFS::DataOutput SQLiteFS::get(const std::string& name) const {
    return m_impl->get(name);
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

SQLiteFS::~SQLiteFS() {} // NOLINT
