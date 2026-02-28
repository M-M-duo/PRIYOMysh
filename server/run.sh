#!/bin/bash
set -a
source "$(dirname "$0")/.env"
set +a
"$(dirname "$0")/build/drogon_app"
