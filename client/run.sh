#!/bin/bash

set -e

RUN_CMAKE=true
BUILD_TESTS=false

for arg in "$@"; do
    if [ "$arg" == "-nb" ]; then
        RUN_CMAKE=false
    elif [ "$arg" == "-t" ]; then
        BUILD_TESTS=true
    fi
done

if [ "$RUN_CMAKE" = true ]; then
    echo "=== Configuring project with CMake ==="
    rm -rf build
    mkdir build
    cd build
    cmake ..
else
    if [ ! -d "build" ] || [ ! -f "build/Makefile" ]; then
        echo "Error: Build cache does not exist. Run without -nb first to run CMake."
        exit 1
    fi
    cd build
fi

if [ "$BUILD_TESTS" = true ]; then
    echo "=== Building EVERYTHING (Project + Tests) ==="
    make -j$(nproc)
else
    echo "=== Building MAIN APP ONLY (Skipping tests) ==="
    make ping_monitor -j$(nproc)
fi

echo "=== Starting the app ==="
env -i DISPLAY=$DISPLAY XAUTHORITY=$XAUTHORITY ./ping_monitor