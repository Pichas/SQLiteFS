include(FetchContent)

FetchContent_Declare(
    libassert
    GIT_REPOSITORY https://github.com/jeremy-rifkin/libassert.git
    GIT_TAG        v2.1.0
)

FetchContent_MakeAvailable(libassert)
