cmake_minimum_required(VERSION 2.8)

project(libsensor C)

#file(GLOB HEADERS source/*.h source/services/*.h)
file(GLOB SOURCES source/*.c source/services/*.c)
include_directories("include")

add_definitions(-D_GNU_SOURCE)

add_library(${PROJECT_NAME} SHARED ${HEADERS} ${SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
