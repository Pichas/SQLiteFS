#include "sqlitefs.h"

#include <utility>
#include "utils.h"


namespace
{
const inline std::vector<std::string> INIT_DB{
  R"query(
        CREATE TABLE IF NOT EXISTS "fs" (
            "id"        INTEGER,
            "parent"    INTEGER,
            "name"      TEXT NOT NULL,
            "ext"       TEXT,
            "attrib"    INTEGER DEFAULT 0,
            "size"      INTEGER,
            "size_raw"  INTEGER,
            PRIMARY KEY("id" AUTOINCREMENT),
            UNIQUE("parent","name"),
            CONSTRAINT "parent_fk" FOREIGN KEY("parent") REFERENCES "fs"("id") ON UPDATE CASCADE ON DELETE CASCADE
        )
    )query",

  R"query(
        CREATE TABLE IF NOT EXISTS "data" (
            "id"    INTEGER,
            "data"  BLOB NOT NULL,
            PRIMARY KEY("id"),
            CONSTRAINT "file_id" FOREIGN KEY("id") REFERENCES "fs"("id") ON UPDATE CASCADE ON DELETE CASCADE
        )
    )query",

  R"query(
        CREATE TABLE IF NOT EXISTS "shortcut" (
            "name"  TEXT,
            "path"  INTEGER,
            PRIMARY KEY("name")
            CONSTRAINT "sc_fk" FOREIGN KEY("path") REFERENCES "fs"("id") ON UPDATE CASCADE ON DELETE CASCADE
        )
    )query",

  R"query(INSERT OR IGNORE INTO fs ("id", "name") VALUES ('0','/'))query",
};

const inline std::string PWD = R"query(
        SELECT concat('/', group_concat(n, '/')) FROM (
            WITH RECURSIVE
            pwd(i, p, n) AS (
                SELECT id, parent, name FROM fs WHERE id IS ? and parent not NULL
                UNION ALL
                SELECT id, parent, name FROM fs, pwd WHERE fs.id IS pwd.p and parent not NULL
            )
            SELECT * FROM pwd ORDER BY i ASC
        )
    )query";


const inline std::string LS            = R"query(SELECT name FROM fs WHERE parent IS ?)query";
const inline std::string GET_ID        = R"query(SELECT id FROM fs WHERE parent IS ? AND name IS ?)query";
const inline std::string GET_PARENT_ID = R"query(SELECT parent FROM fs WHERE id IS ?)query";
const inline std::string RM            = R"query(DELETE FROM fs WHERE parent IS ? AND name IS ?)query";
const inline std::string MKDIR         = R"query(INSERT INTO fs (parent, name) VALUES (?, ?))query";

const inline std::string TOUCH         = R"query(INSERT INTO fs (parent, name, attrib) VALUES (?, ?, 1))query";
const inline std::string SET_FILE_DATA = R"query(INSERT INTO data (id, data) VALUES (?, ?))query";
const inline std::string GET_FILE_DATA = R"query(SELECT data FROM data WHERE id IS ?)query";

} // namespace

SQLiteFS::SQLiteFS(std::string path)
  : m_path(std::move(path)), m_db(m_path, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE) {
    SQLite::Transaction transaction(m_db);

    for (const auto& q : INIT_DB) {
        m_db.exec(q);
    }

    transaction.commit();
}

std::vector<Byte> SQLiteFS::compress(std::span<const Byte> source) {
    uLongf            destination_length = compressBound(source.size());
    std::vector<Byte> destination(destination_length);

    auto ec = compress2(destination.data(), &destination_length, source.data(), source.size(), Z_BEST_COMPRESSION);
    if (ec == Z_OK) {
        destination.resize(destination_length);
        return destination;
    }
    spdlog::critical("Can't compress data");
    return {};
}

std::vector<Byte> SQLiteFS::decompress(std::span<const Byte> source, uLongf destination_length) {
    std::vector<Byte> destination(destination_length);

    int ec = uncompress(destination.data(), &destination_length, source.data(), source.size());
    if (ec == Z_OK) {
        destination.resize(destination_length);
        return destination;
    }
    spdlog::critical("Can't uncompress data");
    return {};
}


std::string SQLiteFS::pwd() const {
    std::string result;

    TRY {
        std::lock_guard   lock(m_mutex);
        SQLite::Statement query{m_db, PWD};
        query.bind(1, static_cast<int64_t>(m_cwd));
        if (query.executeStep()) {
            result = query.getColumn(0).getString();
        }
    };
    return result;
}

std::vector<std::string> SQLiteFS::ls() const {
    std::vector<std::string> content;

    TRY {
        std::lock_guard   lock(m_mutex);
        SQLite::Statement query{m_db, LS};
        query.bind(1, static_cast<int64_t>(m_cwd));
        while (query.executeStep()) {
            content.emplace_back(query.getColumn(0).getString());
        }
    };
    return content;
}

bool SQLiteFS::mkdir(const std::string& name) {
    bool result = false;

    TRY {
        std::lock_guard   lock(m_mutex);
        SQLite::Statement query{m_db, MKDIR};
        query.bind(1, static_cast<int64_t>(m_cwd));
        query.bind(2, name);
        result = query.exec();
    };
    return result;
}

bool SQLiteFS::cd(const std::string& name) {
    bool result = false;

    TRY {
        std::lock_guard   lock(m_mutex);
        SQLite::Statement query{m_db, name == ".." ? GET_PARENT_ID : GET_ID};
        query.bind(1, static_cast<int64_t>(m_cwd));
        if (name != "..") {
            query.bind(2, name);
        }

        if (query.executeStep()) {
            m_cwd  = query.getColumn(0).getInt();
            result = true;
        }
    };
    return result;
}

bool SQLiteFS::rm(const std::string& name) {
    bool result = false;

    TRY {
        std::lock_guard   lock(m_mutex);
        SQLite::Statement query{m_db, RM};
        query.bind(1, static_cast<int64_t>(m_cwd));
        query.bind(2, name);
        result = query.exec();
    };
    return result;
}

bool SQLiteFS::put(const std::string& name, std::span<const Byte> data) {
    auto result = TRY {
        auto compressed = compress(data);

        std::lock_guard     lock(m_mutex);
        SQLite::Transaction transaction(m_db);

        std::int32_t affected = 0;

        { // create empty file if doesn't exist
            SQLite::Statement query{m_db, TOUCH};
            query.bind(1, static_cast<int64_t>(m_cwd));
            query.bind(2, name);
            affected = query.exec();
        }

        if (affected == 0) {
            throw SQLite::Exception("can't touch a file");
        }

        std::int32_t id = 0;
        { // get file id
            SQLite::Statement query{m_db, GET_ID};
            query.bind(1, static_cast<int64_t>(m_cwd));
            query.bind(2, name);

            if (query.executeStep()) {
                id = query.getColumn(0).getInt();
            }
        }

        if (id == 0) {
            throw SQLite::Exception("can't create a file");
        }

        { // save data in the db
            SQLite::Statement query{m_db, SET_FILE_DATA};
            query.bind(1, static_cast<int64_t>(id));
            query.bind(2, compressed.data(), compressed.size());
            query.exec();
        }

        transaction.commit();
        return true;
    };

    return result ? *result : false;
}

std::vector<Byte> SQLiteFS::get(const std::string& name) {

    auto result = TRY->std::string {
        std::lock_guard lock(m_mutex);

        std::int32_t id = 0;
        { // get file id
            SQLite::Statement query{m_db, GET_ID};
            query.bind(1, static_cast<int64_t>(m_cwd));
            query.bind(2, name);

            if (query.executeStep()) {
                id = query.getColumn(0).getInt();
            }
        }

        if (id == 0) {
            throw SQLite::Exception("can't create a file id");
        }

        { // load data from the db
            SQLite::Statement query{m_db, GET_FILE_DATA};
            query.bind(1, static_cast<int64_t>(id));

            if (query.executeStep()) {
                return query.getColumn(0).getString();
            }
        }

        throw SQLite::Exception("can't read a file data");
    };

    if (result) {
        auto span = std::span{reinterpret_cast<const Bytef*>(result->data()), result->size()};
        return decompress(span, 1000);
    }

    return {};
}
