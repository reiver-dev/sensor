#!/bin/bash


export LD_LIBRARY_PATH=:/home/reiver/DEVELOP/epic-ids/epicids/libsensor/bin
bin/sensord -i wlan0 -p -b 65536 --disable-persistance --disable-redirect --disable-fork

