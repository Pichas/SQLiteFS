option(SQLITE3MC_STATIC "Build SQLITE3MC as a static library" ON)
option(SQLITE3MC_BUILD_SHELL "Builds the SQLITE3MC Shell" OFF)
OPTION(SQLITE3MC_SECURE_MEMORY "Enable pragma to secure freed memory" OFF)
OPTION(SQLITE_SECURE_DELETE "Enable secure delete to overwrite deleted content with zeros" OFF)

# AES128 AES256 CHACHA20 SQLCIPHER RC4 ASCON128 AEGIS
set(CODEC_TYPE AES256 CACHE STRING "Set default codec type")
set(SQLITE_THREADSAFE 0 CACHE STRING "Set threading mode (0 = single-threaded, 1 = serialized, 2 = multi-threaded)")

AddExternalPackage(
    sqlite3mc
        v2.1.0
        https://github.com/utelle/SQLite3MultipleCiphers.git
        NO
        NO
)
