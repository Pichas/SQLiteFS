cmake_minimum_required(VERSION 3.20)

project(SQLiteFSLib LANGUAGES  CXX)

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

option(BUILD_SHARED_LIBS "" OFF)

add_subdirectory(external)
find_package(ZLIB REQUIRED)


set(HEADERS
    sqlitefs/sqlitefs.h
)

set(SRC
)

set(SOURCE_FILES
    ${HEADERS}
    ${SRC}
)

add_executable(
    ${PROJECT_NAME}
        main.cpp
)


target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME}
    PUBLIC
        spdlog
        SQLiteCpp sqlite3
        ZLIB::ZLIB
)
