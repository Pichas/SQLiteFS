#include "sqlitefs.h"


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

  R"query(INSERT OR IGNORE INTO "main"."fs" ("id", "name") VALUES ('0','root'))query",
};


//
} // namespace

SQLiteFS::SQLiteFS() : m_db("./data.db", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE) {
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
