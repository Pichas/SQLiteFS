if(TARGET Tracy::TracyClient)
    return()
endif()

message(STATUS "external: downloading target 'Tracy::TracyClient'")

include(FetchContent)
FetchContent_Declare(
    tracy_git
    GIT_REPOSITORY https://github.com/wolfpld/tracy.git
    GIT_TAG v0.11.1
)

option(TRACY_STATIC "Whether to build Tracy as a static library" ON)
option(TRACY_ENABLE "Enable profiling" ON)
option(TRACY_NO_EXIT "Client executable does not exit until all profile data is sent to server" OFF)

FetchContent_MakeAvailable(tracy_git)


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