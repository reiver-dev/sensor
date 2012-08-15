cmake_minimum_required(VERSION 2.8)

project(libsensor C)

add_definitions(-std=c99 -D_GNU_SOURCE)

file(GLOB HEADERS source/*.h source/services/*.h)
file(GLOB SOURCES source/*.c source/services/*.c)
include_directories("include")

add_library(${PROJECT_NAME} SHARED ${HEADERS} ${SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
target_link_libraries (${PROJECT_NAME} pthread pcap zmq)