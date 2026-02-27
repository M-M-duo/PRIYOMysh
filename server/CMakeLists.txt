cmake_minimum_required(VERSION 3.10)
project(priyomysh_2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

find_package(Drogon REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(PostgreSQL REQUIRED)  

find_path(LIBXCRYPT_INCLUDE_DIR
    NAMES crypt.h
    PATHS /opt/homebrew/Cellar/libxcrypt/*/include
    NO_DEFAULT_PATH
)
if(NOT LIBXCRYPT_INCLUDE_DIR)
    message(FATAL_ERROR "crypt.h not found. Please install libxcrypt: brew install libxcrypt")
endif()
include_directories(${LIBXCRYPT_INCLUDE_DIR})

find_library(LIBXCRYPT_LIBRARY
    NAMES crypt libcrypt xcrypt libxcrypt
    PATHS /opt/homebrew/Cellar/libxcrypt/*/lib
    NO_DEFAULT_PATH
)
if(NOT LIBXCRYPT_LIBRARY)
    message(FATAL_ERROR "libxcrypt library not found")
endif()


add_executable(drogon_app
    main.cpp
    controllers/AuthController.cpp
)

target_link_libraries(drogon_app PRIVATE
    Drogon::Drogon
    ${OPENSSL_LIBRARIES}
    ${LIBPQ_LIBRARIES} 
    ${LIBXCRYPT_LIBRARY}
    pthread
)