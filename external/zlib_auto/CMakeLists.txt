if(TARGET ZLIB::ZLIB)
    return()
endif()

if(WIN32)
    message(STATUS "external: downloading target 'ZLIB::ZLIB'")

    include(FetchContent)
    FetchContent_Declare(
        zlib_git
                GIT_REPOSITORY https://gitlab.com/p34ch-main/cmake-zlib-windows-helper.git
                GIT_TAG 1.3.1
        )

    FetchContent_MakeAvailable(zlib_git)
endif()
