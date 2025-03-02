#include <sqlitefs/sqlitefs.h>
#include <SQLiteCpp/SQLiteCpp.h>
#include "sqlitefs_impl.h"


#ifdef MZ_ENABLE
#include "compression.h"

#ifdef HAVE_BZIP
#include <mz_strm_bzip.h>
#endif

#ifdef HAVE_LZMA
#include <mz_strm_lzma.h>
#endif

#ifdef HAVE_ZLIB
#include <mz_strm_zlib.h>
#endif

#ifdef HAVE_ZSTD
#include <mz_strm_zstd.h>
#endif

#endif // MZ_ENABLE


SQLiteFS::SQLiteFS(std::string path, std::string_view key) : m_impl(std::make_unique<Impl>(std::move(path), key)) {
    SQLiteFS::registerSaveFunc("raw", [](DataInput data) { return DataOutput{data.begin(), data.end()}; });
    SQLiteFS::registerLoadFunc("raw", [](DataInput data) { return DataOutput{data.begin(), data.end()}; });

#ifdef HAVE_BZIP
    SQLiteFS::registerSaveFunc("bzip", [](DataInput data) { return minizipCompress(data, mz_stream_bzip_create); });
    SQLiteFS::registerLoadFunc("bzip", [](DataInput data) { return minizipDecompress(data, mz_stream_bzip_create); });
#endif

#ifdef HAVE_LZMA
    SQLiteFS::registerSaveFunc("lzma", [](DataInput data) { return minizipCompress(data, mz_stream_lzma_create); });
    SQLiteFS::registerLoadFunc("lzma", [](DataInput data) { return minizipDecompress(data, mz_stream_lzma_create); });
#endif

#ifdef HAVE_ZLIB
    SQLiteFS::registerSaveFunc("zlib", [](DataInput data) { return minizipCompress(data, mz_stream_zlib_create); });
    SQLiteFS::registerLoadFunc("zlib", [](DataInput data) { return minizipDecompress(data, mz_stream_zlib_create); });
#endif

#ifdef HAVE_ZSTD
    SQLiteFS::registerSaveFunc("zstd", [](DataInput data) { return minizipCompress(data, mz_stream_zstd_create); });
    SQLiteFS::registerLoadFunc("zstd", [](DataInput data) { return minizipDecompress(data, mz_stream_zstd_create); });
#endif
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

bool SQLiteFS::write(const std::string& name, DataInput data, const std::string& alg) {
    return m_impl->write(name, data, alg);
}

SQLiteFS::DataOutput SQLiteFS::read(const std::string& name) const {
    return m_impl->read(name);
}

bool SQLiteFS::mv(const std::string& from, const std::string& to) {
    return m_impl->mv(from, to);
}

bool SQLiteFS::cp(const std::string& from, const std::string& to) {
    return m_impl->cp(from, to);
}

const std::string& SQLiteFS::path() const noexcept {
    return m_impl->path();
}

void SQLiteFS::vacuum() {
    m_impl->vacuum();
}

std::string SQLiteFS::error() const {
    return m_impl->error();
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

void SQLiteFS::rawCall(const std::function<void(SQLite::Database*)>& callback) {
    m_impl->rawCall(callback);
}

SQLiteFS::~SQLiteFS() {} // NOLINT
