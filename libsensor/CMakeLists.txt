cmake_minimum_required(VERSION 2.8)

project(libsensor C)

file(GLOB HEADERS headers/*.h) 
file(GLOB SOURCES source/*.c)
include_directories("headers")

add_definitions(-D_GNU_SOURCE)

add_library(${PROJECT_NAME} SHARED ${HEADERS} ${SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
