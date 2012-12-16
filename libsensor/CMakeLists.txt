cmake_minimum_required(VERSION 2.8)

project(libsensor CXX)

add_definitions(-std=gnu++11)

file(GLOB HEADERS source/*.hpp)
file(GLOB SOURCES source/*.cpp)

macro(putlibpath header_path lib_path)
    set(INCLUDES ${INCLUDES} ${header_path})
    set(LIBS ${LIBS} ${lib_path})
endmacro(putlibpath)

macro(putlib header lib)
    find_path(HDR ${header})
    find_library(LIB ${lib})
    putlibpath(${HDR} ${LIB})
endmacro(putlib)

putlib(pcap.h pcap)
putlib(ev.h ev)
putlib(msgpack.hpp msgpack)

find_path(PGM_H NAMES pgm/pgm.h PATH_SUFFIXES pgm-5.1 pgm-5.2)
find_library(LIBPGM pgm)
putlibpath(${PGM_H} ${LIBPGM})


include_directories(include ${INCLUDES})

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")

target_link_libraries (${PROJECT_NAME} pthread ${LIBS})
