cmake_minimum_required(VERSION 2.8)

project(libsensor CXX)

set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} -std=gnu99)
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -std=gnu++11)

file(GLOB HEADERS source/*.hpp source/*.h source/*/*.hpp source/*/*.h)
file(GLOB SOURCES source/*.cpp source/*.c source/*/*.cpp source/*/*.c)

macro(putlibpath header_path lib_path)
    set(INCLUDES ${INCLUDES} ${header_path})
    set(LIBS ${LIBS} ${lib_path})
    message("Found library: ${header_path} ${lib_path}")
endmacro(putlibpath)

macro(putlib name header_name lib_name)
    find_path(${name}_INCLUDE ${header_name})
    find_library(${name}_LIBS ${lib_name})
    putlibpath(${${name}_INCLUDE} ${${name}_LIBS})
endmacro(putlib)

putlib(PCAP pcap.h pcap)
putlib(EV ev.h ev)
#putlib2(MSGPACK msgpack.hpp msgpack)

find_path(PGM_INCLUDE NAMES pgm/pgm.h PATH_SUFFIXES pgm-5.1 pgm-5.2)
find_library(PGM_LIBS pgm)
putlibpath(${PGM_INCLUDE} ${PGM_LIBS})

set(LOCAL_INCLUDES source source/net)

include_directories(include ${INCLUDES} ${LOCAL_INCLUDES})

add_library(${PROJECT_NAME} SHARED ${HEADERS} ${SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")

target_link_libraries (${PROJECT_NAME} pthread ${LIBS})