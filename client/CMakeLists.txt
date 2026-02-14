cmake_minimum_required(VERSION 3.16)
project(ping_monitor VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)


find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

add_executable(ping_monitor
    main.cpp
    mainwindow.cpp
    mainwindow.h 
)

target_link_libraries(ping_monitor
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
)

set_target_properties(ping_monitor PROPERTIES
    OUTPUT_NAME "ping_monitor"
    MACOSX_BUNDLE FALSE
    WIN32_EXECUTABLE FALSE
)
