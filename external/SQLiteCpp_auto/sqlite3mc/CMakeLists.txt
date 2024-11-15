if(TARGET sqlite3mc)
    return()
endif()

message(STATUS "external: downloading target 'sqlite3mc'")

include(FetchContent)
FetchContent_Declare(
    sqlitemc_git
    GIT_REPOSITORY https://github.com/utelle/SQLite3MultipleCiphers.git
    GIT_TAG        v1.9.0
    PATCH_COMMAND git apply ${CMAKE_CURRENT_SOURCE_DIR}/fix_cmake_file.cmake.patch
    UPDATE_DISCONNECTED 1
)

option(SQLITE3MC_STATIC "Build SQLITE3MC as a static library" ON)
option(SQLITE3MC_BUILD_SHELL "Builds the SQLITE3MC Shell" OFF)
OPTION(SQLITE3MC_SECURE_MEMORY "Enable pragma to secure freed memory" OFF)

# AES128 AES256 CHACHA20 SQLCIPHER RC4 ASCON128
set(CODEC_TYPE AES256 CACHE STRING "Set default codec type")

set(SQLITE_THREADSAFE 0 CACHE STRING "Set threading mode (0 = single-threaded, 1 = serialized, 2 = multi-threaded)")

FetchContent_MakeAvailable(sqlitemc_git)
