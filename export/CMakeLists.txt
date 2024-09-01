if(TARGET sqlitefs::sqlitefs)
    return()
endif()

message(STATUS "external: making target 'sqlitefs::sqlitefs'")

project(sqlitefs LANGUAGES CXX)

add_library(${PROJECT_NAME} STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/includes/sqlitefs/sqlitefs.h
)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        debug ${CMAKE_CURRENT_SOURCE_DIR}/Debug/spdlogd.lib
        debug ${CMAKE_CURRENT_SOURCE_DIR}/Debug/sqlite3.lib
        debug ${CMAKE_CURRENT_SOURCE_DIR}/Debug/SQLiteCpp.lib
        debug ${CMAKE_CURRENT_SOURCE_DIR}/Debug/SQLiteFS.lib

        optimized ${CMAKE_CURRENT_SOURCE_DIR}/Release/spdlog.lib
        optimized ${CMAKE_CURRENT_SOURCE_DIR}/Release/sqlite3.lib
        optimized ${CMAKE_CURRENT_SOURCE_DIR}/Release/SQLiteCpp.lib
        optimized ${CMAKE_CURRENT_SOURCE_DIR}/Release/SQLiteFS.lib
    )
endif()

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/includes)
set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)