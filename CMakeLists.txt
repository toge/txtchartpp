cmake_minimum_required(VERSION 3.25)
project(txtchartpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3")
endif()

add_library(txtchartpp INTERFACE)
target_include_directories(txtchartpp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(txtchartpp INTERFACE cxx_std_26)

find_package(Catch2 REQUIRED CONFIG)
include(CTest)
include(Catch)

add_executable(test_plot
    test/test_plot.cpp
    test/test_braille.cpp
    test/test_bar.cpp
    test/test_bar_braille.cpp
)
target_link_libraries(test_plot PRIVATE txtchartpp Catch2::Catch2WithMain)
target_compile_features(test_plot PRIVATE cxx_std_26)

catch_discover_tests(test_plot)
