#!/bin/bash


gdbserver localhost:31337 build/bin/sensord -i wlan0 -p -b 65536 --disable-persistance --disable-redirect --disable-fork --config "~/SENSOR"
