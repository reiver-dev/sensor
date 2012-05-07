#!/bin/bash

MEMCHECK_SETTINGS="--tool=memcheck --leak-check=full --show-reachable=yes --track-origins=yes"
COMMON_SETTINGS="--track-fds=yes -v"
OUTPUT_SETTINGS="--log-file=valgrind-memcheck.log"

valgrind $MEMCHECK_SETTINGS $COMMON_SETTINGS $OUTPUT_SETTINGS build/bin/sensord --config sensor.conf
#valgrind --tool=massif build/bin/sensord --config sensor.conf