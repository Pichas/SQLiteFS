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

    const std::string& path() const noexcept { return m_db_path; }

    bool mkdir(const std::string& full_path) {
        const auto& [path_id, name] = getPathAndName(full_path);

        if (!path_id) {
            return false;
        }

        auto result = TRY {
            std::lock_guard   lock(m_mutex);
            SQLite::Statement query{m_db, MKDIR};
            query.bind(1, *path_id);
            query.bind(2, name);
            return query.exec();
        };
        return result ? *result : false;
    }


    bool cd(const std::string& path) {
        if (auto node = pathResolver(path); node) {
            m_cwd = *node;
            return true;
        }
        return false;
    }

    bool rm(const std::string& full_path) {
        const auto& [path_id, name] = getPathAndName(full_path);

        if (!path_id) {
            return false;
        }

        auto result = TRY {
            std::lock_guard   lock(m_mutex);
            SQLite::Statement query{m_db, RM};
            query.bind(1, *path_id);
            query.bind(2, name);
            return query.exec();
        };
        return result ? *result : false;
    }

    std::string pwd() const {
        auto result = TRY {
            std::lock_guard   lock(m_mutex);
            SQLite::Statement query{m_db, PWD};
            query.bind(1, m_cwd);
            if (query.executeStep()) {
                return query.getColumn(0).getString();
            }
            throw SQLite::Exception("Folder doesn't exist");
        };

        return result ? *result : "Folder doesn't exist";
    }

    std::vector<SQLiteFSNode> ls(const std::string& path) const {
        const auto& [path_id, name] = getPathAndName(path + "/");
        std::vector<SQLiteFSNode> content;

        if (!path_id) {
            return content;
        }

        TRY {
            std::lock_guard   lock(m_mutex);
            SQLite::Statement query{m_db, LS};
            query.bind(1, *path_id);

            SQLiteFSNode node;
            while (getNodeData(query, node)) {
                content.emplace_back(node);
            }
        };
        return content;
    }

    bool put(const std::string& full_path, DataInput data, const std::string& alg) {
        const auto& [path_id, name] = getPathAndName(full_path);

        if (!path_id) {
            return false;
        }

        auto result = TRY {
            auto data_modified = internalCall(alg, data, m_save_funcs);

            std::lock_guard     lock(m_mutex);
            SQLite::Transaction transaction(m_db);

            { // create empty file if doesn't exist
                SQLite::Statement query{m_db, TOUCH};
                query.bind(1, *path_id);
                query.bind(2, name);
                query.bind(3, static_cast<std::int64_t>(data_modified.size())); // doesn't support uint64_t
                query.bind(4, static_cast<std::int64_t>(data.size()));          // doesn't support uint64_t
                query.bind(5, alg);

                if (!query.exec()) {
                    throw SQLite::Exception("can't touch a file");
                }
            }

            std::uint32_t id = 0;
            { // get file id
                SQLite::Statement query{m_db, GET_ID};
                query.bind(1, *path_id);
                query.bind(2, name);

                if (query.executeStep()) {
                    id = query.getColumn(0).getInt();
                } else {
                    throw SQLite::Exception("can't get file id");
                }
            }

            assert(id);

            { // save data in the db
                SQLite::Statement query{m_db, SET_FILE_DATA};
                query.bind(1, id);
                query.bind(2, data_modified.data(), data_modified.size());
                if (!query.exec()) {
                    throw SQLite::Exception("can't save file data");
                }
            }

            transaction.commit();
            return true;
        };

        return result ? *result : false;
    }

    DataOutput get(const std::string& full_path) const {
        const auto& [path_id, name] = getPathAndName(full_path);

        if (!path_id) {
            return {};
        }

        auto result = TRY {
            std::lock_guard lock(m_mutex);

            auto id = getFileId(*path_id, name);
            if (!id) {
                throw SQLite::Exception("can't get file id");
            }

            SQLiteFSNode fs_node;
            { // get file id
                SQLite::Statement query{m_db, GET_INFO};
                query.bind(1, *id);
                getNodeData(query, fs_node);
            }

            if (fs_node.id == 0) {
                throw SQLite::Exception("can't get file info");
            }


            { // load data from the db
                SQLite::Statement query{m_db, GET_FILE_DATA};
                query.bind(1, *id);

                if (query.executeStep()) {
                    return std::make_pair(fs_node, query.getColumn(0).getString());
                }
            }

            throw SQLite::Exception("can't read file data");
        };

        if (result) {
            const auto& [node, data] = *result;

            auto        span = std::span{reinterpret_cast<const std::uint8_t*>(data.data()), data.size()};
            const auto& file = internalCall(node.compression, span, m_load_funcs);
            if (static_cast<std::size_t>(node.size_raw) != file.size()) {
                spdlog::error("File size doesn't mach.\nFS - {}, File - {}", node.size_raw, file.size());
            }
            return file;
        }

        return {};
    }


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

    static bool getNodeData(SQLite::Statement& query, SQLiteFSNode& out) {
        if (query.executeStep()) {
            out.id          = query.getColumn(0).getInt();
            out.parent_id   = query.getColumn(1).getInt();
            out.name        = query.getColumn(2).getString();
            auto attrib     = query.getColumn(3).getInt();
            out.size        = query.getColumn(4).getInt();
            out.size_raw    = query.getColumn(5).getInt();
            out.compression = query.getColumn(6).getString();
            out.is_file     = attrib & (1 << 0);
            return true;
        }
        return false;
    }

    std::optional<std::uint32_t> getFileId(std::uint32_t path_id, const std::string& name) const {
        SQLite::Statement query{m_db, GET_ID};
        query.bind(1, path_id);
        query.bind(2, name);

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

        return TRY {
            std::lock_guard lock(m_mutex);
            std::uint32_t   id = path.starts_with('/') ? ROOT : m_cwd;

            auto names = split(path, '/');
            for (const auto& name : names) {
                if (name == "." || name == "") {
                    continue;
                }

                SQLite::Statement query{m_db, name == ".." ? GET_PARENT_ID : GET_ID};
                query.bind(1, id);
                if (name != "..") {
                    query.bind(2, name);
                }

                if (query.executeStep()) {
                    id = query.getColumn(0).getInt();
                } else {
                    throw SQLite::Exception("Path doesn't exist");
                }
            }
            return id;
        };
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
    std::uint32_t            m_cwd = 0; // 0 - root
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
