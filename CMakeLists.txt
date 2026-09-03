cmake_minimum_required(VERSION 3.25)

set(TXTCHARTPP_VERSION "1.0.0")

project(txtchartpp VERSION ${TXTCHARTPP_VERSION} LANGUAGES CXX)

option(BUILD_TEST "Enable tests" ON)
option(ENABLE_WASI_MINIMAL "Enable WASI minimal (no exceptions) mode - wasi-sdk wasm32-wasip1 assumed" OFF)
option(ENABLE_FREESTANDING "Define TXTCHARTPP_FREESTANDING and add the freestanding check test (requires BUILD_TEST)" OFF)

if(ENABLE_WASI_MINIMAL)
    add_compile_definitions(TXTCHARTPP_WASI_MINIMAL=1)
endif()

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3")
endif()

add_library(txtchartpp INTERFACE)
add_library(txtchartpp::txtchartpp ALIAS txtchartpp)
# MSVC (cl.exe) には /std:c++26 が存在せず、CMake も cl.exe 向けに
# cxx_std_26 feature を提供しないため /std:c++latest で代替する。
if(MSVC)
    target_compile_options(txtchartpp INTERFACE $<$<CXX_COMPILER_ID:MSVC>:/std:c++latest>)
else()
    target_compile_features(txtchartpp INTERFACE cxx_std_26)
endif()

target_include_directories(txtchartpp INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if(BUILD_TEST)
    enable_testing()
    add_subdirectory(test)
endif()

install(TARGETS txtchartpp
    EXPORT txtchartppTargets
)
install(DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/include/txtchartpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/txtchartppConfigVersion.cmake"
    VERSION ${TXTCHARTPP_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/txtchartppConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/txtchartppConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/txtchartpp"
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/txtchartppConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/txtchartppConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/txtchartpp"
)

install(EXPORT txtchartppTargets
    FILE txtchartppTargets.cmake
    NAMESPACE txtchartpp::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/txtchartpp"
)
