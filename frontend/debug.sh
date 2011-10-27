#!/bin/bash


export LD_LIBRARY_PATH=:/home/reiver/DEVELOP/epic-ids/epicids/libsensor/bin
gdbserver localhost:31337 bin/mockapp
