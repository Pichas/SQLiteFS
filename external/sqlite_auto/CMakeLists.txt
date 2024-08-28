if(TARGET SQLiteCpp)
    return()
endif()

message(STATUS "external: downloading target 'SQLiteCpp'")

include(FetchContent)
FetchContent_Declare(
  sqlite_git
  GIT_REPOSITORY https://github.com/SRombauts/SQLiteCpp.git
  GIT_TAG        3.3.2
)

option(SQLITECPP_RUN_CPPLINT "Run cpplint.py tool for Google C++ StyleGuide." OFF)
option(SQLITECPP_INCLUDE_SCRIPT "Include config & script files." OFF)
option(SQLITECPP_RUN_CPPCHECK "Run cppcheck C++ static analysis tool." OFF)

FetchContent_MakeAvailable(sqlite_git)
target_compile_options(SQLiteCpp PRIVATE "-Wno-language-extension-token")
