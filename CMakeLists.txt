cmake_minimum_required(VERSION 3.21)
project(DiceSimulatorQt VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# The convolution in the core is compute-heavy: default to Release (-O2),
# otherwise an untyped `cmake -B build` silently runs at -O0.
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets Concurrent)
qt_standard_project_setup()

qt_add_executable(DiceSimulatorQt
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/chartwidget.cpp
    src/chartwidget.h
    src/dice.cpp
    src/dice.h
)
target_link_libraries(DiceSimulatorQt PRIVATE
    Qt6::Widgets
    Qt6::Concurrent
)
qt_add_resources(DiceSimulatorQt "assets"
    PREFIX "/"
    FILES assets/dice_d20_icon_.ico
)

# --- Тесты ядра (без Qt) ---
enable_testing()
add_executable(test_dice
    tests/test_dice.cpp
    src/dice.cpp
    src/dice.h
)
target_include_directories(test_dice PRIVATE src)
add_test(NAME dice_tests COMMAND test_dice)
