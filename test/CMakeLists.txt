add_executable(smoke_wasi_minimal smoke_wasi_minimal.cpp)
target_link_libraries(smoke_wasi_minimal PRIVATE txtchartpp::txtchartpp)
target_compile_definitions(smoke_wasi_minimal PRIVATE TXTCHARTPP_WASI_MINIMAL=1)
# WASI_MINIMAL の契約 (例外なしでビルド可能) を hosted でも検証する
target_compile_options(smoke_wasi_minimal PRIVATE $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-fno-exceptions>)
add_test(NAME smoke_wasi_minimal COMMAND smoke_wasi_minimal)

if(ENABLE_WASI_MINIMAL)
  # WASI_MINIMAL ビルドでは hosted な Catch2 テストはスキップ
  return()
endif()

find_package(Catch2 REQUIRED CONFIG)
include(Catch)

add_executable(test_plot
    test_plot.cpp
    test_braille.cpp
    test_bar.cpp
    test_bar_block.cpp
)
target_link_libraries(test_plot PRIVATE txtchartpp::txtchartpp Catch2::Catch2WithMain)

# catch_discover_tests はテスト名を argv で渡すため、Windows では
# 日本語テスト名が文字化けして "No test cases matched" になる。
# MSVC ではバイナリ全体を 1 テストとして登録する (ケース毎の粒度は捨てる)。
if(MSVC)
    add_test(NAME test_plot COMMAND test_plot)
else()
    catch_discover_tests(test_plot)
endif()
