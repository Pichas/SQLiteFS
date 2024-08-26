include(FetchContent)

FetchContent_Declare(
  sqlite
  GIT_REPOSITORY https://github.com/SRombauts/SQLiteCpp.git
  GIT_TAG        3.3.1
)

option(SQLITECPP_RUN_CPPLINT "" OFF)

FetchContent_MakeAvailable(sqlite)
