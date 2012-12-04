cmake_minimum_required(VERSION 2.8)

project(libsensor CXX)

add_definitions(-std=gnu++11)

file(GLOB HEADERS source/*.hpp)
file(GLOB SOURCES source/*.cpp)

set(CMAKE_FIND_LIBRARY_SUFFIXES .a)

find_path(PCAP_H pcap.h)
find_library(LIBPCAP pcap)

find_path(EV_H ev.h)
find_library(LIBEV ev)

include_directories(include ${PCAP_H} ${EV_H})

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
target_link_libraries (${PROJECT_NAME} pthread ${LIBPCAP} ${LIBEV})