cmake_minimum_required (VERSION 2.8)

project(epic-ids)

set(EXECUTABLE_OUTPUT_PATH ../bin)
set(LIBRARY_OUTPUT_PATH ../bin)

set(CFLAGS -pipe -Wall -pedantic)

if(DEBUG)
    set(CFLAGS ${CFLAGS} -g3 -DDEBUG)
else()
    set(CFLAGS ${CFLAGS} -O2)
endif()

add_definitions(${CFLAGS})

add_subdirectory(libsensor)
add_subdirectory(frontend)



