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
option(ENABLE_FREESTANDING "Define TXTCHARTPP_FREESTANDING and add the freestanding check test (requires BUILD_TEST)" OFF)

add_library(txtchartpp INTERFACE)
target_include_directories(txtchartpp INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
# MSVC (cl.exe) には /std:c++26 が存在せず、CMake も cl.exe 向けに
# cxx_std_26 feature を提供しないため /std:c++latest で代替する。
if(MSVC)
    target_compile_options(txtchartpp INTERFACE $<$<CXX_COMPILER_ID:MSVC>:/std:c++latest>)
else()
    target_compile_features(txtchartpp INTERFACE cxx_std_26)
endif()

if(BUILD_TEST)
    find_package(Catch2 REQUIRED CONFIG)
    include(CTest)
    include(Catch)

    add_executable(test_plot
        test/test_plot.cpp
        test/test_braille.cpp
        test/test_bar.cpp
        test/test_bar_block.cpp
    )
    target_link_libraries(test_plot PRIVATE txtchartpp Catch2::Catch2WithMain)
    # C++26 は txtchartpp INTERFACE 側の指定 (MSVC は /std:c++latest) に従う

    # catch_discover_tests はテスト名を argv で渡すため、Windows では
    # 日本語テスト名が文字化けして "No test cases matched" になる。
    # MSVC ではバイナリ全体を 1 テストとして登録する (ケース毎の粒度は捨てる)。
    if(MSVC)
        add_test(NAME test_plot COMMAND test_plot)
    else()
        catch_discover_tests(test_plot)
    endif()

    if(ENABLE_FREESTANDING)
        # hosted 実行検証: to_chars フォールバック実装の動作確認。
        # (<string>/<vector> を使うため hosted ビルドでは -ffreestanding を付けない)
        add_executable(freestanding_check test/freestanding_check.cpp)
        target_link_libraries(freestanding_check PRIVATE txtchartpp)
        target_compile_definitions(freestanding_check PRIVATE TXTCHARTPP_FREESTANDING)
        add_test(NAME freestanding_check COMMAND freestanding_check)

        # wasm32-unknown-unknown コンパイル検証 (ヘッダの自動有効化 +
        # hosted 専用 <format> への依存が無いこと)。
        # clang は emsdk 同梱のものを優先して検索し、libc++ ヘッダは emsdk の
        # sysroot を使う。見つからなければスキップする。
        find_program(TXTCHARTPP_WASM_CLANG NAMES clang++ HINTS "$ENV{EMSDK}/upstream/bin")
        if(TXTCHARTPP_WASM_CLANG)
            set(TXTCHARTPP_WASM_SYSROOT "$ENV{EMSDK}/upstream/emscripten/cache/sysroot"
                CACHE PATH "wasm32 freestanding ビルド用 sysroot (libc++ ヘッダを含むこと)")
            if(EXISTS "${TXTCHARTPP_WASM_SYSROOT}/include/c++/v1/string")
                add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/freestanding_check_wasm.o
                    COMMAND ${TXTCHARTPP_WASM_CLANG} --target=wasm32-unknown-unknown
                        -std=c++20 -Os -fno-exceptions -fno-rtti -nostdlib
                        -I${CMAKE_CURRENT_SOURCE_DIR}/include
                        -isystem${TXTCHARTPP_WASM_SYSROOT}/include/c++/v1
                        -isystem${TXTCHARTPP_WASM_SYSROOT}/include/wasm32-wasi
                        -isystem${TXTCHARTPP_WASM_SYSROOT}/include
                        -c ${CMAKE_CURRENT_SOURCE_DIR}/test/freestanding_check.cpp
                        -o ${CMAKE_CURRENT_BINARY_DIR}/freestanding_check_wasm.o
                    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/include/txtchartpp/txtchart.hpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/test/freestanding_check.cpp
                    COMMENT "wasm32-unknown-unknown freestanding compile check (txtchartpp)"
                )
                add_custom_target(freestanding_check_wasm
                    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/freestanding_check_wasm.o)
            else()
                message(WARNING "TXTCHARTPP_WASM_SYSROOT に libc++ ヘッダが見つからないため wasm32 検証をスキップします: ${TXTCHARTPP_WASM_SYSROOT}")
            endif()
        else()
            message(STATUS "clang++ (emsdk) が見つからないため wasm32 freestanding 検証をスキップします")
        endif()
    endif()
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
