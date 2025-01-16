# more info https://github.com/cpm-cmake/CPM.cmake
option(USE_CPM "Use CPM to manage deps" OFF)


if(USE_CPM)
    if(WIN32)
        set(HOME_VAR USERPROFILE)
    endif()
    if(LINUX)
        set(HOME_VAR HOME)
    endif()

    if(NOT CPM_SOURCE_CACHE)
        cmake_path(SET CPM_SOURCE_CACHE $ENV{${HOME_VAR}}/.cached_deps)
    endif()

    # SPDX-License-Identifier: MIT
    #
    # SPDX-FileCopyrightText: Copyright (c) 2019-2023 Lars Melchior and contributors
    set(CPM_DOWNLOAD_VERSION 0.40.5)
    set(CPM_HASH_SUM "c46b876ae3b9f994b4f05a4c15553e0485636862064f1fcc9d8b4f832086bc5d")
    if(CPM_SOURCE_CACHE)
        set(CPM_DOWNLOAD_LOCATION "${CPM_SOURCE_CACHE}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
    elseif(DEFINED ENV{CPM_SOURCE_CACHE})
        set(CPM_DOWNLOAD_LOCATION "$ENV{CPM_SOURCE_CACHE}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
    else()
        set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
    endif()

    # Expand relative path. This is important if the provided path contains a tilde (~)
    get_filename_component(CPM_DOWNLOAD_LOCATION ${CPM_DOWNLOAD_LOCATION} ABSOLUTE)
    file(DOWNLOAD
        https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake
        ${CPM_DOWNLOAD_LOCATION} EXPECTED_HASH SHA256=${CPM_HASH_SUM}
    )
    include(${CPM_DOWNLOAD_LOCATION})
endif(USE_CPM)


function(AddExternalPackage CURRENT_TARGET CURRENT_VERSION CURRENT_REPO DOWNLOAD_ONLY PATCH_NAME)
    if(TARGET ${CURRENT_TARGET})
        return()
    endif()
    message(STATUS "external: downloading target '${CURRENT_TARGET}'")

    if(NOT PATCH_NAME STREQUAL "NO")
        set(PATCH_NAME ${CMAKE_CURRENT_SOURCE_DIR}/${PATCH_NAME})
    endif()

    if(USE_CPM)
        if(PATCH_NAME)
            set(PATCH_CMD ${PATCH_NAME})
        endif()
        CPMAddPackage(
            NAME ${CURRENT_TARGET}
            GIT_REPOSITORY ${CURRENT_REPO}
            GIT_TAG        ${CURRENT_VERSION}
            PATCHES        ${PATCH_CMD}
            DOWNLOAD_ONLY  ${DOWNLOAD_ONLY}
        )
    else()
        if(PATCH_NAME)
            set(PATCH_CMD git apply ${PATCH_NAME})
        endif()
        include(FetchContent)
        FetchContent_Declare(
            ${CURRENT_TARGET}
            GIT_REPOSITORY ${CURRENT_REPO}
            GIT_TAG        ${CURRENT_VERSION}
            PATCH_COMMAND  ${PATCH_CMD}
            UPDATE_DISCONNECTED 1
        )
        FetchContent_MakeAvailable(${CURRENT_TARGET})
    endif()

    set(${CURRENT_TARGET}_POPULATED ${${CURRENT_TARGET}_POPULATED} PARENT_SCOPE)
    set(${CURRENT_TARGET}_SOURCE_DIR ${${CURRENT_TARGET}_SOURCE_DIR} PARENT_SCOPE)
    set(${CURRENT_TARGET}_BINARY_DIR ${${CURRENT_TARGET}_BINARY_DIR} PARENT_SCOPE)
endfunction()


# add subdir
set(CMAKE_FOLDER_BACKUP ${CMAKE_FOLDER})
set(CMAKE_FOLDER External)

file(GLOB V_GLOB LIST_DIRECTORIES true "*_auto")
foreach(item ${V_GLOB})
    if(IS_DIRECTORY ${item})
        add_subdirectory(${item})
    endif()
endforeach()

set(CMAKE_FOLDER ${CMAKE_FOLDER_BACKUP})
