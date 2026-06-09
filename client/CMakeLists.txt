cmake_minimum_required(VERSION 3.16)
project(ping_monitor VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Test)

add_executable(ping_monitor
    main.cpp
    mainwindow.cpp
    authdialog.cpp
    postdialog.cpp
    feedwindow.cpp
    friendfinder.cpp
    editprofiledialog.cpp
    mainwindow.h
    authdialog.h
    postdialog.h
    feedwindow.h
    friendfinder.h
    editprofiledialog.h
    resources.qrc
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

enable_testing()

# Перечисляем все файлы тестов и тестируемые исходники
add_executable(ping_monitor_tests
    tests/test_main.cpp
    tests/test_mainwindow.cpp
    tests/test_authdialog.cpp
    tests/test_postdialog.cpp
    tests/test_editprofiledialog.cpp
    tests/test_feedwindow.cpp
    tests/test_friendfinder.cpp
    
    # Исходные файлы логики для тестирования
    mainwindow.cpp
    authdialog.cpp
    postdialog.cpp
    editprofiledialog.cpp
    feedwindow.cpp
    friendfinder.cpp
)

target_link_libraries(ping_monitor_tests PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Test
)

add_test(NAME PriyomyshAllTests COMMAND ping_monitor_tests)