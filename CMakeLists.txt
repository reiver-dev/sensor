cmake_minimum_required (VERSION 2.8)

project(epic-ids)

INCLUDE (CheckIncludeFiles)
CHECK_INCLUDE_FILES (future ATOMIC_INT_LOCK_FREE)

set(EXECUTABLE_OUTPUT_PATH ../bin)
set(LIBRARY_OUTPUT_PATH ../bin)

add_definitions(-pipe -Wall -pedantic)

if(DEBUG)
    add_definitions(-g3)
else()
    add_definitions(-O2)
endif()

if (STATIC)
	add_definitions(-static)
	set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
endif()

add_definitions(${CFLAGS})

include_directories(libsensor/include)
include_directories(msgqueue)

add_subdirectory(libsensor)
add_subdirectory(frontend)



