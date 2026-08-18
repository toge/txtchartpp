cmake_minimum_required(VERSION 3.25)
project(txtchartpp LANGUAGES CXX VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)

if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3")
endif()

option(BUILD_TEST "Enable tests" ON)

add_library(txtchartpp INTERFACE)
target_include_directories(txtchartpp INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_features(txtchartpp INTERFACE cxx_std_26)

if(BUILD_TEST)
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
endif()

# ---- Install ----

include(CMakePackageConfigHelpers)

install(
    DIRECTORY include/txtchartpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    TARGETS txtchartpp
    EXPORT txtchartppTargets
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    EXPORT txtchartppTargets
    FILE txtchartppTargets.cmake
    NAMESPACE txtchartpp::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/txtchartpp
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/txtchartppConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/txtchartppConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/txtchartpp
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/txtchartppConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/txtchartppConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/txtchartppConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/txtchartpp
)
