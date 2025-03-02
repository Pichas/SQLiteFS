cmake_minimum_required(VERSION 3.20)

project(sqlitefs LANGUAGES CXX VERSION 1.1.11 DESCRIPTION "In sqlite file system")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Use solution folders to organize projects
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(BUILD_SHARED_LIBS "Build shared libraries (DLLs)" OFF)

option(SQLITEFS_BUILD_TESTS "Build tests." ${PROJECT_IS_TOP_LEVEL})
option(SQLITEFS_ENABLE_PROFILER "Enable Tracy profiler library" OFF)
option(SQLITEFS_BUILD_PROFILER_SERVER "Build Tracy profiler exe. Can be built with CL only" OFF)
option(SQLITEFS_OVERRIDE_GLOBAL_ALLOCATORS "Override global allocators to track memory in Tracy profiler" OFF)

option(MZ_ENABLE "Enables minizip lib" OFF)
option(MZ_ZLIB "Enables ZLIB compression" OFF)
option(MZ_BZIP2 "Enables BZIP2 compression" OFF)
option(MZ_LZMA "Enables LZMA & XZ compression" OFF)
option(MZ_ZSTD "Enables ZSTD compression" OFF)

option(TRACY_ENABLE "Enable profiling" ON)
option(TRACY_STATIC "Whether to build Tracy as a static library" ON)
option(TRACY_NO_EXIT "Client executable does not exit until all profile data is sent to server" OFF)

set(CODEC_TYPE AES256 CACHE STRING "Set default codec type")


add_library(${PROJECT_NAME})
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})


if(SQLITEFS_ENABLE_PROFILER)
    target_compile_definitions(${PROJECT_NAME} PUBLIC SQLITEFS_PROFILER_ENABLED)
    target_link_libraries(${PROJECT_NAME} PUBLIC TracyClient)

    option(TRACY_BUILD_PROFILER "Build tracy profiler exe" ${SQLITEFS_BUILD_PROFILER_SERVER})

    if(SQLITEFS_OVERRIDE_GLOBAL_ALLOCATORS)
        target_compile_definitions(${PROJECT_NAME} PUBLIC SQLITEFS_ALLOCATORS_ENABLED)
    endif()
endif()


option(USE_CPM "Use CPM to manage deps" ${PROJECT_IS_TOP_LEVEL})
add_subdirectory(external)


set(HEADERS
    includes/sqlitefs/sqlitefs.h
)

set(HEADERS_PRIVATE
    sqlitefs/sqlqueries.h
    sqlitefs/utils.h
    sqlitefs/sqlitefs_impl.h
)

set(SRC
    sqlitefs/sqlitefs.cpp
    sqlitefs/profiler/profiler.cpp
    sqlitefs/sqlitefs_impl.cpp
)


if(MZ_ENABLE)
    list(APPEND HEADERS_PRIVATE sqlitefs/compression.h)
    target_compile_definitions(${PROJECT_NAME} PRIVATE MZ_ENABLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE minizip)

    if(MZ_ZLIB)
        # zlib1g-dev
        target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_ZLIB)
    endif()

    if(MZ_LZMA)
        # liblzma-dev
        target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_LZMA)
    endif()

    if(MZ_BZIP2)
        target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_BZIP)
    endif()

    if(MZ_ZSTD)
        # libzstd-dev
        target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_ZSTD)
    endif()
endif()

set(SOURCE_FILES
    ${HEADERS}
    ${HEADERS_PRIVATE}
    ${SRC}
)

target_sources(${PROJECT_NAME} PRIVATE ${SOURCE_FILES})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/includes)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/sqlitefs)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
target_link_libraries(${PROJECT_NAME} PRIVATE SQLiteCpp)
set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER ${HEADERS}
)

if (SQLITEFS_BUILD_TESTS)
    message("SQLiteFS tests are enabled")
    include(CTest)
    add_subdirectory(tests)
endif(SQLITEFS_BUILD_TESTS)
