#pragma once

#include <SQLiteCpp/SQLiteCpp.h>
#include <spdlog/spdlog.h>
#include <zlib.h>
#include "sqlitefs/sqlitefs.h"
#include "utils.h"


// compression algs
inline SQLiteFS::DataOutput zlibCompress(SQLiteFS::DataInput source) {
    uLongf               destination_length = compressBound(source.size());
    SQLiteFS::DataOutput destination(destination_length);

    auto ec = compress2(destination.data(), &destination_length, source.data(), source.size(), Z_BEST_COMPRESSION);
    if (ec == Z_OK) {
        destination.resize(destination_length);
        return destination;
    }
    spdlog::critical("Can't compress data");
    return {};
}

inline SQLiteFS::DataOutput zlibDecompress(SQLiteFS::DataInput source) {
    uLongf               destination_length = 0;
    SQLiteFS::DataOutput destination;

    int ec = 0;
    do {
        destination_length += 128_MiB;
        destination.resize(destination_length, 0);
        ec = uncompress(destination.data(), &destination_length, source.data(), source.size());
    } while (ec == Z_BUF_ERROR);

    if (ec == Z_OK) {
        destination.resize(destination_length);
        return destination;
    }

    spdlog::critical("Can't uncompress data");
    return {};
}
