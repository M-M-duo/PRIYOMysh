#!/bin/bash
set -a
source "$(dirname "$0")/.env"
set +a

export DYLD_LIBRARY_PATH=/opt/homebrew/opt/postgresql@14/lib:$DYLD_LIBRARY_PATH

cd "$(dirname "$0")/build" || exit
mkdir -p logs
./drogon_app