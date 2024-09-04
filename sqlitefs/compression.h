#pragma once

#include <mz.h>
#include <mz_os.h>
#include <mz_strm.h>
#include <mz_strm_mem.h>
#include <mz_strm_os.h>
#include <SQLiteCpp/SQLiteCpp.h>
#include "sqlitefs/sqlitefs.h"


inline SQLiteFS::DataOutput minizipCompress(SQLiteFS::DataInput source, mz_stream_create_cb create_compress) {
    std::size_t result = 0;

    /* raw data into memory stream */
    void* raw_stream = mz_stream_mem_create();
    assert(raw_stream != nullptr);

    result = mz_stream_mem_open(raw_stream, nullptr, MZ_OPEN_MODE_CREATE);
    assert(result == MZ_OK);

    result = mz_stream_write(raw_stream, source.data(), source.size());
    assert(result == source.size());


    /* Compress data into memory stream */
    void* compress_stream = mz_stream_mem_create();
    assert(compress_stream != nullptr);

    result = mz_stream_mem_open(compress_stream, nullptr, MZ_OPEN_MODE_CREATE);
    assert(result == MZ_OK);

    void* deflate_stream = create_compress();
    assert(deflate_stream != nullptr);
    mz_stream_set_base(deflate_stream, compress_stream);


    /* Copy data from file stream and write to compression stream */
    mz_stream_seek(raw_stream, 0, MZ_SEEK_SET);
    mz_stream_open(deflate_stream, nullptr, MZ_OPEN_MODE_WRITE);
    mz_stream_copy_stream_to_end(deflate_stream, nullptr, raw_stream, nullptr);
    mz_stream_close(deflate_stream);

    int64_t total_in = 0;
    mz_stream_get_prop_int64(deflate_stream, MZ_STREAM_PROP_TOTAL_IN, &total_in);
    assert(total_in == mz_stream_tell(raw_stream));

    int64_t total_out = 0;
    mz_stream_get_prop_int64(deflate_stream, MZ_STREAM_PROP_TOTAL_OUT, &total_out);
    assert(total_out == mz_stream_tell(compress_stream));

    mz_stream_delete(&deflate_stream);

    SQLiteFS::DataOutput compressed(total_out);

    mz_stream_seek(compress_stream, 0, MZ_SEEK_SET);
    result = mz_stream_read(compress_stream, compressed.data(), compressed.size());
    assert(compressed.size() == result);

    mz_stream_mem_close(compress_stream);
    mz_stream_mem_delete(&compress_stream);
    mz_stream_mem_close(raw_stream);
    mz_stream_mem_delete(&raw_stream);
    return compressed;
}


inline SQLiteFS::DataOutput minizipDecompress(SQLiteFS::DataInput source, mz_stream_create_cb create_compress) {
    std::size_t result = 0;

    /* raw data into memory stream */
    void* raw_stream = mz_stream_mem_create();
    assert(raw_stream != nullptr);

    result = mz_stream_mem_open(raw_stream, nullptr, MZ_OPEN_MODE_CREATE);
    assert(result == MZ_OK);

    result = mz_stream_write(raw_stream, source.data(), source.size());
    assert(result == source.size());


    /* Decompress data into memory stream */
    void* uncompress_stream = mz_stream_mem_create();
    assert(uncompress_stream != nullptr);

    result = mz_stream_mem_open(uncompress_stream, nullptr, MZ_OPEN_MODE_CREATE);
    assert(result == MZ_OK);

    mz_stream_seek(raw_stream, 0, MZ_SEEK_SET);
    // mz_stream_seek(compress_stream, 0, MZ_SEEK_SET);

    void* inflate_stream = create_compress();
    assert(inflate_stream != nullptr);
    mz_stream_set_base(inflate_stream, raw_stream); // NOLINT

    mz_stream_open(inflate_stream, nullptr, MZ_OPEN_MODE_READ);
    mz_stream_copy_stream_to_end(uncompress_stream, nullptr, inflate_stream, nullptr);
    mz_stream_close(inflate_stream);

    int64_t total_in = 0;
    mz_stream_get_prop_int64(inflate_stream, MZ_STREAM_PROP_TOTAL_IN, &total_in);
    assert(total_in == mz_stream_tell(raw_stream));

    int64_t total_out = 0;
    mz_stream_get_prop_int64(inflate_stream, MZ_STREAM_PROP_TOTAL_OUT, &total_out);
    assert(total_out == mz_stream_tell(uncompress_stream));

    mz_stream_delete(&inflate_stream);

    SQLiteFS::DataOutput uncompressed(total_out);

    mz_stream_seek(uncompress_stream, 0, MZ_SEEK_SET);
    result = mz_stream_read(uncompress_stream, uncompressed.data(), uncompressed.size());
    assert(uncompressed.size() == result);

    mz_stream_mem_close(uncompress_stream);
    mz_stream_mem_delete(&uncompress_stream);
    mz_stream_mem_close(raw_stream);
    mz_stream_mem_delete(&raw_stream);
    return uncompressed;
}
