project(SQLiteFSTests LANGUAGES  CXX)

set(CMAKE_FOLDER_BACKUP ${CMAKE_FOLDER})
set(CMAKE_FOLDER External)

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set(CMAKE_FOLDER ${CMAKE_FOLDER_BACKUP})

add_executable(${PROJECT_NAME}  ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        GTest::gtest_main
        SQLiteFS
)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})
