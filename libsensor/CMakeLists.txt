cmake_minimum_required(VERSION 2.8)

project(libsensor CXX)

add_definitions(-std=gnu++11)

file(GLOB HEADERS source/*.hpp)
file(GLOB SOURCES source/*.cpp)

find_path(PCAP_H pcap)
find_library(LIBPCAP pcap)

include_directories(include ${PCAP_H})

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
target_link_libraries (${PROJECT_NAME} pthread ${LIBPCAP})