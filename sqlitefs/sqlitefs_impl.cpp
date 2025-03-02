#include "sqlitefs_impl.h"
#include <cstring>
#include <mutex>
#include <optional>
#include <SQLiteCpp/SQLiteCpp.h>
#include "sqlitefs/sqlitefs.h"
#include "sqlqueries.h"
#include "utils.h"


namespace
{
struct SecureString final : public std::string {
    using std::string::basic_string;
    ~SecureString() { std::memset(data(), 0, size()); };
};

SQLiteFS::DataOutput internalCall(const std::string&               name,
                                  SQLiteFS::DataInput              data,
                                  const SQLiteFS::ConvertFuncsMap& map) {
    auto it = map.find(name);
    if (it != map.end()) {
        return std::invoke(it->second, data);
    }
    assert(false && "Function doesn't exist");
    return {};
}

} // namespace

template<typename... Args>
int SQLiteFS::Impl::exec(const std::string& query_string, Args&&... args) {
    SQLITEFS_SCOPED_PROFILER;
    using namespace std::literals;

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
SQLite::Statement SQLiteFS::Impl::select(const std::string& query_string, Args&&... args) const {
    SQLITEFS_SCOPED_PROFILER;

    SQLite::Statement query{m_db, query_string};

    [&]<size_t... Is>(std::index_sequence<Is...>) {
        (query.bind(Is + 1, std::forward<Args>(args)), ...);
    }(std::make_index_sequence<sizeof...(Args)>{});
    return query;
}

SQLiteFS::Impl::Impl(std::string path, std::string_view key)
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

bool SQLiteFS::Impl::mkdir(const std::string& full_path) {
    SQLITEFS_SCOPED_PROFILER;

    std::lock_guard lock(m_mutex);

    const auto& [path_id, name] = splitPathAndName(full_path);
    if (!path_id) {
        m_last_error = "Can't find path";
        return false;
    }

    return exec(MKDIR, *path_id, name);
}

bool SQLiteFS::Impl::cd(const std::string& path) {
    SQLITEFS_SCOPED_PROFILER;

    std::lock_guard lock(m_mutex);

    auto path_id = resolve(path);
    if (!path_id) {
        return false;
    }

    auto n = node(*path_id);
    if (n && !(n->attributes & SQLiteFSNode::Attributes::FILE)) {
        m_cwd = n->id;
        return true;
    }

    m_last_error = "Can't find path";
    return false;
}

bool SQLiteFS::Impl::rm(const std::string& path) {
    SQLITEFS_SCOPED_PROFILER;

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
        m_cwd = SQLITEFS_ROOT;
    }

    return result;
}

std::string SQLiteFS::Impl::pwd() const {
    SQLITEFS_SCOPED_PROFILER;

    std::lock_guard lock(m_mutex);

    auto query = select(PWD, m_cwd);
    return query.executeStep() ? query.getColumn(0).getString() : "";
}

std::vector<SQLiteFSNode> SQLiteFS::Impl::ls(const std::string& path) const {
    SQLITEFS_SCOPED_PROFILER;

    std::vector<SQLiteFSNode> content;
    std::lock_guard           lock(m_mutex);

    auto current_node = node(path + "/");
    if (!current_node) {
        return content;
    }

    if (!(current_node->attributes & SQLiteFSNode::Attributes::FILE)) {
        auto row_count = select(LS_COUNT, current_node->id);
        if (!row_count.executeStep()) {
            return content;
        }
        content.reserve(row_count.getColumn(0).getUInt());

        auto query = select(LS, current_node->id);

        while (auto child_node = node(query)) {
            content.emplace_back(std::move(*child_node));
        }
        return content;
    }

    content.emplace_back(std::move(*current_node));
    return content;
}

bool SQLiteFS::Impl::write(const std::string& full_path, DataInput data, const std::string& alg) {
    SQLITEFS_SCOPED_PROFILER;

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

SQLiteFS::DataOutput SQLiteFS::Impl::read(const std::string& full_path) const {
    SQLITEFS_SCOPED_PROFILER;
    using namespace std::literals;

    DataOutput       result;
    std::unique_lock lock(m_mutex);

    auto id = resolve(full_path);
    if (!id) {
        return result;
    }

    auto current_node = node(*id);
    if (current_node && (current_node->attributes & SQLiteFSNode::Attributes::FILE)) {
        auto data_query = select(GET_FILE_DATA, *id);
        if (!data_query.executeStep()) {
            assert(false && "internal error: DB is broken. No data for file node");
            return result;
        }

        auto data = data_query.getColumn(0).getString();

        lock.unlock();
        auto   view = std::span{reinterpret_cast<const char*>(data.data()), data.size()};
        auto&& temp = internalCall(current_node->compression, view, m_load_funcs);
        lock.lock();

        if (static_cast<std::size_t>(current_node->size_raw) != temp.size()) {
            m_last_error = "File size doesn't mach.\nFS meta - "s + std::to_string(current_node->size_raw) +
                           ", File - " + std::to_string(temp.size());
        } else {
            result = std::move(temp);
        }
        return result;
    }

    m_last_error = "Can't read folder data";
    return result;
}

bool SQLiteFS::Impl::mv(const std::string& from, const std::string& to) {
    SQLITEFS_SCOPED_PROFILER;

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
        target && (target->attributes & SQLiteFSNode::Attributes::FILE)) {
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

bool SQLiteFS::Impl::cp(const std::string& from, const std::string& to) {
    SQLITEFS_SCOPED_PROFILER;

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
        target && (target->attributes & SQLiteFSNode::Attributes::FILE)) {
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

void SQLiteFS::Impl::vacuum() {
    SQLITEFS_SCOPED_PROFILER;
    std::lock_guard lock(m_mutex);
    exec("VACUUM");
}

std::string SQLiteFS::Impl::error() const {
    std::string temp;
    {
        std::lock_guard lock(m_mutex);
        temp.swap(m_last_error);
    }
    return temp;
};

const std::string& SQLiteFS::Impl::path() const noexcept {
    return m_db_path;
}

void SQLiteFS::Impl::registerSaveFunc(const std::string& name, const ConvertFunc& func) {
    SQLITEFS_SCOPED_PROFILER;
    assert(!m_save_funcs.contains(name));
    m_save_funcs.try_emplace(name, func);
}
void SQLiteFS::Impl::registerLoadFunc(const std::string& name, const ConvertFunc& func) {
    SQLITEFS_SCOPED_PROFILER;
    assert(!m_load_funcs.contains(name));
    m_load_funcs.try_emplace(name, func);
}

SQLiteFS::DataOutput SQLiteFS::Impl::callSaveFunc(const std::string& name, DataInput data) {
    SQLITEFS_SCOPED_PROFILER;
    return internalCall(name, data, m_save_funcs);
}
SQLiteFS::DataOutput SQLiteFS::Impl::callLoadFunc(const std::string& name, DataInput data) {
    SQLITEFS_SCOPED_PROFILER;
    return internalCall(name, data, m_load_funcs);
}

void SQLiteFS::Impl::rawCall(const std::function<void(SQLite::Database*)>& callback) {
    SQLITEFS_SCOPED_PROFILER;
    std::lock_guard lock(m_mutex);
    std::invoke(callback, &m_db);
}


bool SQLiteFS::Impl::saveBlob(std::uint32_t id, DataInput data) {
    SQLITEFS_SCOPED_PROFILER;
    using namespace std::literals;

    try {
        SQLite::Statement query{m_db, SET_FILE_DATA};
        query.bind(1, id);
        query.bind(2, data.data(), data.size());
        return query.exec();
    } catch (std::exception& e) { m_last_error = "SQL Error: "s + e.what(); }
    return false;
}


std::optional<SQLiteFSNode> SQLiteFS::Impl::node(const std::string& path) const {
    SQLITEFS_SCOPED_PROFILER;

    const auto& [path_id, name] = splitPathAndName(path);
    if (!path_id) {
        m_last_error = "Can't find path";
        return std::nullopt;
    }
    return name.empty() ? node(*path_id) : node(*path_id, name);
}

std::optional<SQLiteFSNode> SQLiteFS::Impl::node(std::uint32_t id) const {
    SQLITEFS_SCOPED_PROFILER;

    auto query = select(GET_NODE_BY_ID, id);
    return node(query);
}


std::optional<SQLiteFSNode> SQLiteFS::Impl::node(std::uint32_t path_id, const std::string& name) const {
    SQLITEFS_SCOPED_PROFILER;

    auto query = select(GET_NODE, path_id, name);
    return node(query);
}

std::optional<SQLiteFSNode> SQLiteFS::Impl::node(SQLite::Statement& query) const {
    SQLITEFS_SCOPED_PROFILER;

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

std::optional<std::uint32_t> SQLiteFS::Impl::resolve(const std::string& path) const {
    SQLITEFS_SCOPED_PROFILER;

    if (path.empty()) {
        return m_cwd;
    }

    if (path == "/") {
        return SQLITEFS_ROOT;
    }

    std::uint32_t id    = path.starts_with('/') ? SQLITEFS_ROOT : m_cwd;
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

std::pair<std::optional<std::uint32_t>, std::string> SQLiteFS::Impl::splitPathAndName(
  const std::string& full_path) const {
    SQLITEFS_SCOPED_PROFILER;

    auto pos = full_path.find_last_of('/');

    if (pos == std::string::npos) {
        return {m_cwd, full_path};
    }

    auto path = full_path.substr(0, pos + 1);
    auto name = full_path.substr(pos + 1);
    return {resolve(path), std::move(name)};
}
