cmake_minimum_required (VERSION 2.8)

project (sensord C)

file (GLOB HEADERS headers/*.h)
file (GLOB SOURCES source/*.c)

set (LIBRARIES
    libsensor
    mysqlclient)

include_directories (
    headers
    ../libsensor/include
)

add_executable (${PROJECT_NAME} ${HEADERS} ${SOURCES})
target_link_libraries (${PROJECT_NAME} ${LIBRARIES})
set_target_properties (${PROJECT_NAME} PROPERTIES PREFIX "")
