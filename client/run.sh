#!/bin/bash

set -e

NO_BUILD=false
if [ "$1" = "-nb" ]; then
    cd build
    make -j$(nproc)
    NO_BUILD=true
fi

if [ "$NO_BUILD" = false ]; then
    rm -rf build
    mkdir build
    cd build
    cmake ..
    make -j$(nproc)
else
    if [ ! -d "build" ]; then
        echo "Error: build directory does not exist. Run without -nb first."
        exit 1
    fi
    cd build
    make -j$(nproc)
fi

echo "=== Starting the app ==="
env -i DISPLAY=$DISPLAY XAUTHORITY=$XAUTHORITY ./ping_monitor