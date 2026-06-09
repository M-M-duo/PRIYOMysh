#!/bin/bash

set -e

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

if [ -f "$(dirname "$0")/.env" ]; then
    set -a
    source "$(dirname "$0")/.env"
    set +a
else
    echo -e "${YELLOW}Warning: .env file not found. Using system environment variables.${NC}"
fi

if [ "$(uname -s)" == "Darwin" ]; then
    export DYLD_LIBRARY_PATH=/opt/homebrew/opt/postgresql@14/lib:$DYLD_LIBRARY_PATH
fi

RUN_CMAKE=true
BUILD_TESTS=false

for arg in "$@"; do
    if [ "$arg" == "-nb" ]; then
        RUN_CMAKE=false
    elif [ "$arg" == "-t" ]; then
        BUILD_TESTS=true
    fi
done

if [ "$(uname -s)" == "Darwin" ]; then
    NPROC=$(sysctl -n hw.ncpu)
else
    NPROC=$(nproc)
fi

if [ "$RUN_CMAKE" = true ]; then
    echo -e "${YELLOW}=== Configuring server project with CMake ===${NC}"
    rm -rf build
    mkdir -p build
    cd build
    cmake .. -DCMAKE_BUILD_TYPE=Debug
else
    if [ ! -d "build" ] || [ ! -f "build/Makefile" ]; then
        echo -e "${RED}Error: Build cache does not exist. Run without -nb first.${NC}"
        exit 1
    fi
    cd build
fi

mkdir -p logs

if [ "$BUILD_TESTS" = true ]; then
    echo -e "${YELLOW}=== Building and Running C++ Tests (test_helpers) ===${NC}"
    make test_helpers -j"$NPROC"
    echo -e "${GREEN}=== Launching C++ Helpers Unit Tests ===${NC}"
    ./test_helpers
else
    echo -e "${YELLOW}=== Building Main Application (drogon_app) ===${NC}"
    make drogon_app -j"$NPROC"
    echo -e "${GREEN}=== Starting Drogon Server Application ===${NC}"
    ./drogon_app
fi