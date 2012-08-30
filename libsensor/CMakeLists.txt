cmake_minimum_required(VERSION 2.8)

project(libsensor CXX)

add_definitions(-std=gnu++11)

file(GLOB HEADERS source/*.hpp source/services/*.hpp)
file(GLOB SOURCES source/*.cpp source/services/*.cpp)
include_directories("include")

add_library(${PROJECT_NAME} SHARED ${HEADERS} ${SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
target_link_libraries (${PROJECT_NAME} pthread pcap zmq)