#!/bin/bash


export LD_LIBRARY_PATH=:/home/reiver/DEVELOP/epic-ids/epicids/libsensor/bin
bin/sensord -i wlan0 -p -H 127.0.0.1 -N sniffer -S sniffer -U root -P toor
