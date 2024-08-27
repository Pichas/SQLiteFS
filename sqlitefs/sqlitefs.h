#pragma once

#include <span>
#include <SQLiteCpp/SQLiteCpp.h>
#include <spdlog/spdlog.h>
#include <zconf.h>
#include <zlib.h>


struct SQLiteFS final {
    SQLiteFS(std::string path = "./data.db");


    const std::string& path() const noexcept { return m_path; }

    bool                     mkdir(const std::string& name);
    bool                     cd(const std::string& name);
    bool                     rm(const std::string& name);
    std::string              pwd() const;
    std::vector<std::string> ls() const;

    bool              put(const std::string& name, std::span<const Byte> data);
    std::vector<Byte> get(const std::string& name);

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
    std::vector<Byte> compress(std::span<const Byte> source);
    std::vector<Byte> decompress(std::span<const Byte> source, uLongf destination_length);

private:
    std::string              m_path;
    std::int32_t             m_cwd = 0; // 0 - root
    mutable std::mutex       m_mutex;
    mutable SQLite::Database m_db;
};