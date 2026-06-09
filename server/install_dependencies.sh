#!/bin/bash

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

detect_os() {
    case "$(uname -s)" in
        Linux*)   OS="Linux";;
        Darwin*)  OS="macOS";;
        *)        OS="Unknown";;
    esac
    echo -e "${BLUE}Detected OS: $OS${NC}"
}

install_headers() {
    echo -e "${YELLOW}Checking header-only libraries (jwt-cpp & picojson)...${NC}"

    if [ ! -d "lib/jwt-cpp" ] && [ ! -f "/usr/local/include/jwt-cpp/jwt.h" ]; then
        echo -e "${YELLOW}Installing jwt-cpp globally...${NC}"
        git clone https://github.com/Thalhammer/jwt-cpp.git /tmp/jwt-cpp
        sudo cp -r /tmp/jwt-cpp/include/jwt-cpp /usr/local/include/
        rm -rf /tmp/jwt-cpp
    fi

    if [ ! -f "/usr/local/include/picojson/picojson.h" ]; then
        echo -e "${YELLOW}Installing picojson globally...${NC}"
        git clone https://github.com/kazuho/picojson.git /tmp/picojson
        sudo mkdir -p /usr/local/include/picojson
        sudo cp /tmp/picojson/picojson.h /usr/local/include/picojson/
        rm -rf /tmp/picojson
    fi
}

install_linux() {
    echo -e "${YELLOW}Installing server dependencies for Linux...${NC}"
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        case "$ID" in
            ubuntu|debian)
                sudo apt update
                sudo apt install -y cmake build-essential libjsoncpp-dev libssl-dev libpq-dev uuid-dev postgresql-client git
                ;;
            *)
                echo -e "${RED}Unsupported Linux distribution: $ID. Please install cmake, jsoncpp, openssl, uuid and postgresql-client manually.${NC}"
                exit 1
                ;;
        esac
    fi

    if ! ldconfig -p | grep -q libdrogon; then
        echo -e "${YELLOW}Drogon Framework not found. Building from source...${NC}"
        git clone https://github.com/drogonframework/drogon.git /tmp/drogon
        cd /tmp/drogon
        git submodule update --init
        mkdir -p build && cd build
        cmake .. -DCMAKE_BUILD_TYPE=Release
        make -j$(nproc)
        sudo make install
        cd -
        rm -rf /tmp/drogon
    else
        echo -e "${GREEN}Drogon Framework is already installed.${NC}"
    fi
}

install_macos() {
    echo -e "${YELLOW}Installing server dependencies for macOS...${NC}"
    if ! command -v brew &> /dev/null; then
        echo -e "${RED}Homebrew not found. Please install it first from https://brew.sh/${NC}"
        exit 1
    fi
    brew update
    brew install cmake openssl ossp-uuid postgresql@14 libxcrypt jsoncpp brotli c-ares sqlite

    if brew list drogon &>/dev/null; then
        echo -e "${YELLOW}Removing brew version of Drogon...${NC}"
        brew uninstall drogon
    fi

    echo -e "${YELLOW}Building Drogon Framework from GitHub with PostgreSQL support...${NC}"
    git clone https://github.com/drogonframework/drogon.git /tmp/drogon_mac
    cd /tmp/drogon_mac
    git submodule update --init
    mkdir -p build && cd build
    
    cmake .. \
      -DCMAKE_BUILD_TYPE=Release \
      -DPOSTGRESQL_INCLUDE_DIR=/opt/homebrew/opt/postgresql@14/include \
      -DPOSTGRESQL_LIBRARIES=/opt/homebrew/opt/postgresql@14/lib/libpq.dylib
      
    make -j$(sysctl -n hw.ncpu)
    sudo make install
    cd -
    rm -rf /tmp/drogon_mac
}

detect_os
install_headers

case "$OS" in
    Linux)   install_linux ;;
    macOS)   install_macos ;;
    *)
        echo -e "${RED}Unsupported OS for backend server production.${NC}"
        exit 1
        ;;
esac

echo -e "${GREEN}All server dependencies installed successfully.${NC}"