#pragma once

#include <span>
#include <SQLiteCpp/SQLiteCpp.h>
#include <spdlog/spdlog.h>
#include <zconf.h>
#include <zlib.h>


namespace detail
{
const inline std::vector<std::string> INIT_DB{
  R"query(
        CREATE TABLE IF NOT EXISTS "fs" (
            "id"	INTEGER,
            "parent"	INTEGER,
            "name"	TEXT NOT NULL,
            "ext"	TEXT,
            "attrib"	INTEGER DEFAULT 0,
            "size"	INTEGER,
            "size_raw"	INTEGER,
            PRIMARY KEY("id" AUTOINCREMENT),
            CONSTRAINT "parent_fk" FOREIGN KEY("parent") REFERENCES "fs"("id") ON UPDATE CASCADE ON DELETE CASCADE
        )
    )query",

  R"query(
        CREATE TABLE IF NOT EXISTS "data" (
            "id"	INTEGER NOT NULL UNIQUE,
            "data"	BLOB NOT NULL,
            CONSTRAINT "file_id" FOREIGN KEY("id") REFERENCES "fs"("id") ON UPDATE CASCADE ON DELETE CASCADE
        )
    )query",

  R"query(
        CREATE TABLE IF NOT EXISTS "shortcut" (
            "name"	TEXT NOT NULL UNIQUE,
            "path"	TEXT,
            PRIMARY KEY("name")
        )
    )query",

  R"query(INSERT OR IGNORE INTO "main"."fs" ("name") VALUES ('root'))query", // id 1
  R"query(INSERT OR IGNORE INTO "main"."fs" ("parent", "name") VALUES ('1', 'cubemaps');)query",
  R"query(INSERT OR IGNORE INTO "main"."fs" ("parent", "name") VALUES ('1', 'models');)query",
  R"query(INSERT OR IGNORE INTO "main"."fs" ("parent", "name") VALUES ('1', 'shaders');)query",
  R"query(INSERT OR IGNORE INTO "main"."fs" ("parent", "name") VALUES ('1', 'textures');)query",

  R"query(INSERT OR IGNORE INTO "main"."shortcut" ("name", "path") VALUES ('cubemaps', '/root/cubemaps'))query",
  R"query(INSERT OR IGNORE INTO "main"."shortcut" ("name", "path") VALUES ('models', '/root/models');)query",
  R"query(INSERT OR IGNORE INTO "main"."shortcut" ("name", "path") VALUES ('shaders', '/root/shaders');)query",
  R"query(INSERT OR IGNORE INTO "main"."shortcut" ("name", "path") VALUES ('textures', '/root/textures');)query",
};


//
} // namespace detail

namespace detail::database
{


} // namespace detail::database

struct SQLiteFS final {
    static SQLiteFS& instance() {
        static SQLiteFS instance;
        return instance;
    }

    // void saveChunk(std::size_t id, const std::string& data) {
    //     std::vector<char> compressed = compress(data);

    //     std::lock_guard     lock(m_mutex);
    //     SQLite::Transaction transaction(m_db);

    //     SQLite::Statement query{m_db, "REPLACE INTO chunks VALUES (?, ?)"};
    //     query.bind(1, static_cast<int64_t>(id));
    //     query.bind(2, compressed.data(), compressed.size());
    //     query.exec();

    //     transaction.commit();
    // }

    // std::vector<char> loadChunk(std::size_t id) {
    //     std::vector<char> decompressed;

    //     std::lock_guard   lock(m_mutex);
    //     SQLite::Statement query{m_db, "SELECT value FROM chunks WHERE id = ?"};
    //     query.bind(1, static_cast<int64_t>(id));
    //     while (query.executeStep()) {
    //         auto value   = query.getColumn(0).getString();
    //         decompressed = decompress(value);
    //     }
    //     return decompressed;
    // }

private:
    SQLiteFS() : m_db("./data/data.db", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE) {
        SQLite::Transaction transaction(m_db);

        for (const auto& q : detail::INIT_DB) {
            m_db.exec(q);
        }

        transaction.commit();
    }


    std::vector<Byte> compress(std::span<const Byte> source) {
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

    std::vector<Byte> decompress(std::span<const Byte> source, uLongf destination_length) {
        std::vector<Byte> destination(destination_length);

        int ec = uncompress(destination.data(), &destination_length, source.data(), source.size());
        if (ec == Z_OK) {
            destination.resize(destination_length);
            return destination;
        }
        spdlog::critical("Can't uncompress data");
        return {};
    }

    std::mutex       m_mutex;
    SQLite::Database m_db;
};