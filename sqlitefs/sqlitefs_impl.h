#include <mutex>
#include <optional>
#include <sqlitefs/sqlitefs.h>
#include <SQLiteCpp/SQLiteCpp.h>
#include "utils.h"


constexpr std::uint32_t SQLITEFS_ROOT = 0;

struct SQLiteFS::Impl {
    Impl(std::string path, std::string_view key);

    bool                      mkdir(const std::string& full_path);
    bool                      cd(const std::string& path);
    bool                      rm(const std::string& path);
    std::string               pwd() const;
    std::vector<SQLiteFSNode> ls(const std::string& path) const;
    bool                      write(const std::string& full_path, DataInput data, const std::string& alg);
    DataOutput                read(const std::string& full_path) const;
    bool                      mv(const std::string& from, const std::string& to);
    bool                      cp(const std::string& from, const std::string& to);
    void                      vacuum();
    std::string               error() const;
    const std::string&        path() const noexcept;
    void                      registerSaveFunc(const std::string& name, const ConvertFunc& func);
    void                      registerLoadFunc(const std::string& name, const ConvertFunc& func);
    DataOutput                callSaveFunc(const std::string& name, DataInput data);
    DataOutput                callLoadFunc(const std::string& name, DataInput data);
    void                      rawCall(const std::function<void(SQLite::Database*)>& callback);

private:
    bool                                                 saveBlob(std::uint32_t id, DataInput data);
    std::optional<SQLiteFSNode>                          node(const std::string& path) const;
    std::optional<SQLiteFSNode>                          node(std::uint32_t id) const;
    std::optional<SQLiteFSNode>                          node(std::uint32_t path_id, const std::string& name) const;
    std::optional<SQLiteFSNode>                          node(SQLite::Statement& query) const;
    std::optional<std::uint32_t>                         resolve(const std::string& path) const;
    std::pair<std::optional<std::uint32_t>, std::string> splitPathAndName(const std::string& full_path) const;

private:
    template<typename... Args>
    int exec(const std::string& query_string, Args&&... args);

    template<typename... Args>
    SQLite::Statement select(const std::string& query_string, Args&&... args) const;

private:
    std::string      m_db_path;
    std::uint32_t    m_cwd = SQLITEFS_ROOT;
    SQLite::Database m_db;

    ConvertFuncsMap m_save_funcs;
    ConvertFuncsMap m_load_funcs;

    mutable std::string m_last_error;
    mutable SQLITEFS_LOCABLE_PROFILER(std::mutex, m_mutex);
};
