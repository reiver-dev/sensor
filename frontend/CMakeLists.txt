cmake_minimum_required (VERSION 2.8)

project (sensord C)

add_definitions(-std=c99)

file (GLOB HEADERS source/*.h)
file (GLOB SOURCES source/*.c)

set (LIBRARIES
    libsensor)

include_directories (
    ../libsensor/include
)

add_executable (${PROJECT_NAME} ${HEADERS} ${SOURCES})
target_link_libraries (${PROJECT_NAME} ${LIBRARIES})
set_target_properties (${PROJECT_NAME} PROPERTIES PREFIX "")
