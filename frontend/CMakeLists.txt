cmake_minimum_required (VERSION 2.8)

project (sensord C)

add_definitions(-std=c99)

file (GLOB HEADERS source/*.h)
file (GLOB SOURCES source/*.c)

add_executable (${PROJECT_NAME} ${HEADERS} ${SOURCES})

target_link_libraries (${PROJECT_NAME} libsensor)
