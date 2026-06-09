cmake_minimum_required(VERSION 3.10)
project(priyomysh_2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lib/jwt-cpp/include")
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/jwt-cpp/include)
else()
    include_directories(/usr/local/include)
endif()

find_package(Drogon REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(PostgreSQL REQUIRED)

if (APPLE)
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
else()
    set(LIBXCRYPT_LIBRARY "crypt")
endif()

add_executable(drogon_app
    main.cpp
    controllers/AuthController.cpp
    controllers/ProfileController.cpp
    controllers/FriendsController.cpp
    controllers/PostsController.cpp
)

target_link_libraries(drogon_app PRIVATE
    Drogon::Drogon
    ${OPENSSL_LIBRARIES}
    ${LIBPQ_LIBRARIES} 
    ${LIBXCRYPT_LIBRARY}
)

add_executable(test_helpers
    tests/helpers_test.cpp 
)

target_link_libraries(test_helpers PRIVATE
    Drogon::Drogon
    ${OPENSSL_LIBRARIES}
    ${PostgreSQL_LIBRARIES}
    ${LIBXCRYPT_LIBRARY}
)