add_subdirectory(gtest)

project(SQLiteFSTests LANGUAGES  CXX)

set(CMAKE_FOLDER_BACKUP ${CMAKE_FOLDER})
set(CMAKE_FOLDER Tests/${PROJECT_NAME})

function(InitProject TYPE)
    set(PROJ_NAME ${PROJECT_NAME}_${TYPE})
    add_executable(${PROJ_NAME} ${COMMON_FILES} ${TYPE}_main.cpp )
    target_include_directories(${PROJ_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(${PROJ_NAME}
        PRIVATE
            ${TYPE}_main
            sqlitefs
    )
endfunction()

InitProject(benchmark)
InitProject(gtest)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME}_gtest)

set(CMAKE_FOLDER ${CMAKE_FOLDER_BACKUP})
