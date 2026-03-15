#!/bin/bash

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

detect_os() {
    case "$(uname -s)" in
        Linux*)     OS="Linux";;
        Darwin*)    OS="macOS";;
        CYGWIN*|MINGW*|MSYS*) OS="Windows";;
        *)          OS="Unknown";;
    esac
    echo -e "${BLUE}Detected OS: $OS${NC}"
}

install_linux() {
    echo -e "${YELLOW}Installing dependencies for Linux...${NC}"
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        case "$ID" in
            ubuntu|debian)
                sudo apt update
                sudo apt install -y cmake build-essential qt6-base-dev qt6-network-dev libgl1-mesa-dev
                ;;
            fedora)
                sudo dnf install -y cmake gcc-c++ qt6-qtbase-devel qt6-qtnetwork-devel
                ;;
            arch|manjaro)
                sudo pacman -Syu --noconfirm cmake base-devel qt6-base qt6-network
                ;;
            *)
                echo -e "${RED}Unsupported Linux distribution: $ID${NC}"
                exit 1
                ;;
        esac
    else
        echo -e "${RED}Cannot detect Linux distribution.${NC}"
        exit 1
    fi
}

install_macos() {
    echo -e "${YELLOW}Installing dependencies for macOS...${NC}"
    if ! command -v brew &> /dev/null; then
        echo -e "${YELLOW}Homebrew not found. Installing Homebrew...${NC}"
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    fi
    brew update
    brew install cmake qt@6
    echo 'export PATH="/opt/homebrew/opt/qt@6/bin:$PATH"' >> ~/.zshrc
    echo 'export CMAKE_PREFIX_PATH="/opt/homebrew/opt/qt@6:$CMAKE_PREFIX_PATH"' >> ~/.zshrc
}

install_windows() {
    echo -e "${YELLOW}Windows detected. Please install dependencies manually:${NC}"
    echo -e "1. Install Qt6 from https://www.qt.io/download-qt-installer"
    echo -e "2. Install CMake from https://cmake.org/download/"
    echo -e "3. Install a compiler (MSVC from Visual Studio or MinGW)"
    echo -e "4. Ensure Qt6_DIR is set in CMake or use Qt6 CMake prefix path"
    echo -e "${BLUE}Alternatively, if using MSYS2:${NC}"
    echo -e "   pacman -S mingw-w64-x86_64-cmake mingw-w64-x86_64-gcc mingw-w64-x86_64-qt6"
    echo -e "${BLUE}Or using Chocolatey:${NC}"
    echo -e "   choco install cmake qt6-base"
}

detect_os

case "$OS" in
    Linux)
        install_linux
        ;;
    macOS)
        install_macos
        ;;
    Windows)
        install_windows
        ;;
    *)
        echo -e "${RED}Unsupported OS${NC}"
        exit 1
        ;;
esac

echo -e "${GREEN}All dependencies installed successfully.${NC}"