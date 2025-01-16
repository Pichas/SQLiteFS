# Compatibility options
option(MZ_COMPAT "Enables compatibility layer" ON)
# Compression library options
option(MZ_ZLIB "Enables ZLIB compression" ON)
option(MZ_BZIP2 "Enables BZIP2 compression" ON)
option(MZ_LZMA "Enables LZMA & XZ compression" ON)
option(MZ_ZSTD "Enables ZSTD compression" ON)
option(MZ_LIBCOMP "Enables Apple compression" OFF)
option(MZ_FETCH_LIBS "Enables fetching third-party libraries if not found" ${WIN32})
option(MZ_FORCE_FETCH_LIBS "Enables fetching third-party libraries always" OFF)
# Encryption support options
option(MZ_PKCRYPT "Enables PKWARE traditional encryption" OFF)
option(MZ_WZAES "Enables WinZIP AES encryption" OFF)
option(MZ_OPENSSL "Enables OpenSSL for encryption" OFF)
option(MZ_LIBBSD "Builds with libbsd crypto random" OFF)
# Character conversion options
option(MZ_ICONV "Enables iconv for string encoding conversion" OFF)
# Code generation options
option(MZ_COMPRESS_ONLY "Only support compression" OFF)
option(MZ_DECOMPRESS_ONLY "Only support decompression" OFF)
option(MZ_FILE32_API "Builds using posix 32-bit file api" OFF)
# Build and continuous integration options
option(MZ_BUILD_TESTS "Builds minizip test executable" OFF)
option(MZ_BUILD_UNIT_TESTS "Builds minizip unit test project" OFF)
option(MZ_BUILD_FUZZ_TESTS "Builds minizip fuzzer executables" OFF)
option(MZ_CODE_COVERAGE "Builds with code coverage flags" OFF)

AddExternalPackage(
    minizip
        4.0.7
        https://github.com/zlib-ng/minizip-ng.git
        NO
        NO
)
