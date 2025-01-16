add_subdirectory(sqlite3mc)

option(SQLITECPP_RUN_CPPLINT "Run cpplint.py tool for Google C++ StyleGuide." OFF)
option(SQLITECPP_INCLUDE_SCRIPT "Include config & script files." OFF)
option(SQLITECPP_RUN_CPPCHECK "Run cppcheck C++ static analysis tool." OFF)
option(SQLITECPP_INTERNAL_SQLITE "Add the internal SQLite3 source to the project." OFF)
option(SQLITE_HAS_CODEC "Enable database encryption API. Not available in the public release of SQLite." ON)

set(CURRENT_TARGET SQLiteCpp)
AddExternalPackage(
    ${CURRENT_TARGET}
        3.3.2
        https://github.com/SRombauts/SQLiteCpp.git
        NO
        fix_cmake_file.cmake.patch
)


if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(${CURRENT_TARGET} PRIVATE "-Wno-language-extension-token")
endif()