cmake_minimum_required(VERSION 3.20)

project(sqlitefs LANGUAGES CXX VERSION 1.0.1 DESCRIPTION "In sqlite file system")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Use solution folders to organize projects
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Make sure DLL and EXE targets go to the same directory.
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib) # Output directory for static lib (.LIB)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin) # Output directory for shared lib (.DLL)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin) # Output directory for executables (.EXE)

option(BUILD_SHARED_LIBS "Build shared libraries (DLLs)" OFF)

option(MZ_ENABLE "Enables minizip lib" OFF)
option(MZ_ZLIB "Enables ZLIB compression" OFF)
option(MZ_BZIP2 "Enables BZIP2 compression" OFF)
option(MZ_LZMA "Enables LZMA & XZ compression" OFF)
option(MZ_ZSTD "Enables ZSTD compression" OFF)

set(CODEC_TYPE AES256 CACHE STRING "Set default codec type")

add_subdirectory(external)


set(HEADERS
    includes/sqlitefs/sqlitefs.h
)

set(HEADERS_PRIVATE
    sqlitefs/sqlqueries.h
    sqlitefs/utils.h
)

set(SRC
    sqlitefs/sqlitefs.cpp
)

set(SOURCE_FILES
    ${HEADERS}
    ${HEADERS_PRIVATE}
    ${SRC}
)

if(BUILD_SHARED_LIBS)
    add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})
else()
    add_library(${PROJECT_NAME} ${SOURCE_FILES})
endif(BUILD_SHARED_LIBS)

add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(${PROJECT_NAME} PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})
set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER ${HEADERS})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/includes)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/sqlitefs)
target_link_libraries(${PROJECT_NAME} PRIVATE SQLiteCpp)

if(MZ_ENABLE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE MZ_ENABLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE minizip)

    if(MZ_ZLIB)
        target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_ZLIB)
    endif()

    if(MZ_LZMA)
        target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_LZMA)
    endif()

    if(MZ_BZIP2)
        target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_BZIP)
    endif()

    if(MZ_ZSTD)
        target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_ZSTD)
    endif()
endif()

option(SQLITEFS_BUILD_TESTS "Build tests." ${PROJECT_IS_TOP_LEVEL})
if (SQLITEFS_BUILD_TESTS)
    message("SQLiteFS tests are enabled")
    include(CTest)
    add_subdirectory(tests)
endif(SQLITEFS_BUILD_TESTS)
