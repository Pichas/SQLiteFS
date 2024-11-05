if(TARGET SQLiteCpp)
    return()
endif()

add_subdirectory(sqlite3mc)

message(STATUS "external: downloading target 'SQLiteCpp'")

include(FetchContent)
FetchContent_Declare(
    sqlite_git
    GIT_REPOSITORY https://github.com/SRombauts/SQLiteCpp.git
    GIT_TAG        3.3.2
    PATCH_COMMAND git apply ${CMAKE_CURRENT_SOURCE_DIR}/fix_cmake_file.cmake.patch
    UPDATE_DISCONNECTED 1
)

option(SQLITECPP_RUN_CPPLINT "Run cpplint.py tool for Google C++ StyleGuide." OFF)
option(SQLITECPP_INCLUDE_SCRIPT "Include config & script files." OFF)
option(SQLITECPP_RUN_CPPCHECK "Run cppcheck C++ static analysis tool." OFF)
option(SQLITECPP_INTERNAL_SQLITE "Add the internal SQLite3 source to the project." OFF)
option(SQLITE_HAS_CODEC "Enable database encryption API. Not available in the public release of SQLite." ON)


FetchContent_MakeAvailable(sqlite_git)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(SQLiteCpp PRIVATE "-Wno-language-extension-token")
endif()