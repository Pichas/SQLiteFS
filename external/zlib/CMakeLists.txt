include(FetchContent)

FetchContent_Declare(
    zlib
            GIT_REPOSITORY https://gitlab.com/p34ch-main/cmake-zlib-windows-helper.git
            GIT_TAG 1.3.1
    )

FetchContent_MakeAvailable(zlib)
