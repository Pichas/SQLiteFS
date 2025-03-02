option(TRACY_STATIC "Whether to build Tracy as a static library" ON)
option(TRACY_ENABLE "Enable profiling" ON)
option(TRACY_NO_EXIT "Client executable does not exit until all profile data is sent to server" OFF)


if(SQLITEFS_ENABLE_PROFILER)
    AddExternalPackage(
        TracyClient
            v0.11.1
            https://github.com/wolfpld/tracy.git
            NO
            NO
    )


    option(TRACY_BUILD_PROFILER "Build tracy profiler exe" OFF)

    if(TRACY_BUILD_PROFILER)
        include(ExternalProject)
        ExternalProject_Add(
            profiler
            SOURCE_DIR ${tracy_git_SOURCE_DIR}/profiler
            CMAKE_GENERATOR "Ninja"
            CMAKE_ARGS -DCMAKE_C_COMPILER=cl -DCMAKE_CXX_COMPILER=cl -DCMAKE_BUILD_TYPE=Release -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=${CMAKE_SOURCE_DIR}/build/bin
            BINARY_DIR ${CMAKE_SOURCE_DIR}/build/profiler
            INSTALL_COMMAND ""
            TEST_COMMAND ""
        )
    endif()
endif()
