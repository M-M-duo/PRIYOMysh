#!/bin/bash

set -e 

rm -rf build
mkdir build
cd build
cmake ..
make -j$(nproc)

echo "=== Starting the app ==="
env -i DISPLAY=$DISPLAY XAUTHORITY=$XAUTHORITY ./ping_monitor
