# add subdir
set(CMAKE_FOLDER_BACKUP ${CMAKE_FOLDER})
set(CMAKE_FOLDER External)


# should be included before assimp or be in the system libs
if(WIN32)
	add_subdirectory(zlib)
endif()

find_package(ZLIB REQUIRED)


file(GLOB V_GLOB LIST_DIRECTORIES true "*_auto")
foreach(item ${V_GLOB})
	if(IS_DIRECTORY ${item})
		add_subdirectory(${item})
	endif()
endforeach()

set(CMAKE_FOLDER ${CMAKE_FOLDER_BACKUP})