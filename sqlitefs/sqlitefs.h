#pragma once

#include <span>
#include <SQLiteCpp/SQLiteCpp.h>
#include <spdlog/spdlog.h>
#include <zconf.h>
#include <zlib.h>


#define SQLITEFS SQLiteFS::instance()


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
    SQLiteFS();


    std::vector<Byte> compress(std::span<const Byte> source);
    std::vector<Byte> decompress(std::span<const Byte> source, uLongf destination_length);

    std::mutex       m_mutex;
    SQLite::Database m_db;
    std::size_t      m_cwd = 0; // 0 - root
};