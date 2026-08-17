/**
 * @file test_bar.cpp
 * @brief txtchartpp::bar / txtchartpp::vbar のゴールデンテスト
 * @author toge (toge.mail@gmail.com)
 * @date 2026-08-11
 * @copyright Copyright (c) 2026 toge(toge.mail@gmail.com)
 *
 * @details
 * 期待値は scripts/gen_test_bar.py の参照実装が生成する。
 */
#include <catch2/catch_all.hpp>

#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include "txtchartpp/txtchart.hpp"

using namespace txtchart;

namespace {

/** @brief NaN 値 */
double const nan_value = std::numeric_limits<double>::quiet_NaN();

} // namespace

TEST_CASE("横棒: 基本") {
    auto const series = std::vector<double>{1, 2, 3, 4};
    auto const expected_bar_simple = std::string{
    "       1.00 \xe2\x96\x88\n"
"       2.00 \xe2\x96\x88\xe2\x96\x88\n"
"       3.00 \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\n"
"       4.00 \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88"
  };
    CHECK(bar(series) == expected_bar_simple);
}

TEST_CASE("横棒: 負値") {
    auto const series = std::vector<double>{-3, -2, -1, 0, 1, 2, 3};
    auto const expected_bar_neg = std::string{
    "      -3.00 \n"
"      -2.00 \n"
"      -1.00 \n"
"       0.00 \n"
"       1.00 \xe2\x96\x88\n"
"       2.00 \xe2\x96\x88\xe2\x96\x88\n"
"       3.00 \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88"
  };
    CHECK(bar(series) == expected_bar_neg);
}

TEST_CASE("横棒: 多系列") {
    auto const series = std::vector<std::vector<double>>{
        {10, 20, 30},
        {40, 30, 20},
    };
    auto cfg = Config{};
    cfg.height = 10.0;
    auto const expected_bar_multi = std::string{
    "      10.00 \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\n"
"      40.00 \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\n"
"      20.00 \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\n"
"      30.00 \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\n"
"      30.00 \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\n"
"      20.00 \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88"
  };
    CHECK(bar(series, cfg) == expected_bar_multi);
}

TEST_CASE("横棒: NaN スキップ") {
    auto const series = std::vector<double>{1, 2, nan_value, 4};
    auto const expected_bar_nan = std::string{
    "       1.00 \xe2\x96\x88\n"
"       2.00 \xe2\x96\x88\xe2\x96\x88\n"
"            \n"
"       4.00 \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88"
  };
    CHECK(bar(series) == expected_bar_nan);
}

TEST_CASE("横棒: 一定値") {
    auto const series = std::vector<double>{2.0, 2.0, 2.0};
    auto const expected_bar_flat = std::string{
    "       2.00 \n"
"       2.00 \n"
"       2.00 "
  };
    CHECK(bar(series) == expected_bar_flat);
}

TEST_CASE("横棒: 多系列色付き") {
    auto const series = std::vector<std::vector<double>>{
        {10, 20, 30},
        {40, 30, 20},
    };
    auto cfg = Config{};
    cfg.height = 10.0;
    cfg.colors = {red, blue};
    auto const expected_bar_colors = std::string{
    "      10.00 \x1b[31m\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\x1b[0m\n"
"      40.00 \x1b[34m\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\x1b[0m\n"
"      20.00 \x1b[31m\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\x1b[0m\n"
"      30.00 \x1b[34m\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\x1b[0m\n"
"      30.00 \x1b[31m\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\x1b[0m\n"
"      20.00 \x1b[34m\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\x1b[0m"
  };
    CHECK(bar(series, cfg) == expected_bar_colors);
}

TEST_CASE("縦棒: 基本") {
    auto const series = std::vector<double>{1, 2, 3, 4};
    auto const expected_vbar_simple = std::string{
    "    4.00 \xe2\x94\xa4      \xe2\x96\x88\n"
"    2.50 \xe2\x94\xa4  \xe2\x96\x88 \xe2\x96\x88  \n"
"    1.00 \xe2\x94\xa4\xe2\x96\x88      "
  };
    CHECK(vbar(series) == expected_vbar_simple);
}

TEST_CASE("縦棒: 負値") {
    auto const series = std::vector<double>{-3, -2, -1, 0, 1, 2, 3};
    auto const expected_vbar_neg = std::string{
    "    3.00 \xe2\x94\xa4            \xe2\x96\x88\n"
"    1.80 \xe2\x94\xa4          \xe2\x96\x88 \xe2\x96\x88\n"
"    0.60 \xe2\x94\xbc\xe2\x96\x88 \xe2\x96\x88 \xe2\x96\x88 \xe2\x96\x88 \xe2\x96\x88 \xe2\x96\x88 \xe2\x96\x88\n"
"   -0.60 \xe2\x94\xa4\xe2\x96\x88 \xe2\x96\x88 \xe2\x96\x88        \n"
"   -1.80 \xe2\x94\xa4\xe2\x96\x88 \xe2\x96\x88          \n"
"   -3.00 \xe2\x94\xa4\xe2\x96\x88            "
  };
    CHECK(vbar(series) == expected_vbar_neg);
}

TEST_CASE("縦棒: 多系列") {
    auto const series = std::vector<std::vector<double>>{
        {10, 20, 30},
        {40, 30, 20},
    };
    auto cfg = Config{};
    cfg.height = 6.0;
    auto const expected_vbar_multi = std::string{
    "   40.00 \xe2\x94\xa4 \xe2\x96\x88      \n"
"   34.00 \xe2\x94\xa4        \n"
"   28.00 \xe2\x94\xa4    \xe2\x96\x88 \xe2\x96\x88 \n"
"   22.00 \xe2\x94\xa4   \xe2\x96\x88   \xe2\x96\x88\n"
"   16.00 \xe2\x94\xa4        \n"
"   10.00 \xe2\x94\xa4\xe2\x96\x88       "
  };
    CHECK(vbar(series, cfg) == expected_vbar_multi);
}

TEST_CASE("縦棒: NaN スキップ") {
    auto const series = std::vector<double>{1, 2, nan_value, 4};
    auto const expected_vbar_nan = std::string{
    "    4.00 \xe2\x94\xa4      \xe2\x96\x88\n"
"    2.50 \xe2\x94\xa4  \xe2\x96\x88    \n"
"    1.00 \xe2\x94\xa4\xe2\x96\x88      "
  };
    CHECK(vbar(series) == expected_vbar_nan);
}

TEST_CASE("縦棒: 一定値") {
    auto const series = std::vector<double>{2.0, 2.0, 2.0};
    auto const expected_vbar_flat = std::string{
    "    2.00 \xe2\x94\xa4\xe2\x96\x88 \xe2\x96\x88 \xe2\x96\x88"
  };
    CHECK(vbar(series) == expected_vbar_flat);
}

TEST_CASE("縦棒: 多系列色付き") {
    auto const series = std::vector<std::vector<double>>{
        {10, 20, 30},
        {40, 30, 20},
    };
    auto cfg = Config{};
    cfg.height = 6.0;
    cfg.colors = {red, blue};
    auto const expected_vbar_colors = std::string{
    "   40.00 \xe2\x94\xa4\x1b[31m \x1b[0m\x1b[34m\xe2\x96\x88\x1b[0m \x1b[31m \x1b[0m\x1b[34m \x1b[0m \x1b[31m \x1b[0m\x1b[34m \x1b[0m\n"
"   34.00 \xe2\x94\xa4\x1b[31m \x1b[0m\x1b[34m \x1b[0m \x1b[31m \x1b[0m\x1b[34m \x1b[0m \x1b[31m \x1b[0m\x1b[34m \x1b[0m\n"
"   28.00 \xe2\x94\xa4\x1b[31m \x1b[0m\x1b[34m \x1b[0m \x1b[31m \x1b[0m\x1b[34m\xe2\x96\x88\x1b[0m \x1b[31m\xe2\x96\x88\x1b[0m\x1b[34m \x1b[0m\n"
"   22.00 \xe2\x94\xa4\x1b[31m \x1b[0m\x1b[34m \x1b[0m \x1b[31m\xe2\x96\x88\x1b[0m\x1b[34m \x1b[0m \x1b[31m \x1b[0m\x1b[34m\xe2\x96\x88\x1b[0m\n"
"   16.00 \xe2\x94\xa4\x1b[31m \x1b[0m\x1b[34m \x1b[0m \x1b[31m \x1b[0m\x1b[34m \x1b[0m \x1b[31m \x1b[0m\x1b[34m \x1b[0m\n"
"   10.00 \xe2\x94\xa4\x1b[31m\xe2\x96\x88\x1b[0m\x1b[34m \x1b[0m \x1b[31m \x1b[0m\x1b[34m \x1b[0m \x1b[31m \x1b[0m\x1b[34m \x1b[0m"
  };
    CHECK(vbar(series, cfg) == expected_vbar_colors);
}

TEST_CASE("横棒: 空系列 → 空文字列") {
    CHECK(bar(std::vector<double>{}) == "");
}

TEST_CASE("横棒: 全 NaN → 空文字列") {
    CHECK(bar(std::vector<double>{nan_value, nan_value}) == "");
}

TEST_CASE("縦棒: 空系列 → 空文字列") {
    CHECK(vbar(std::vector<double>{}) == "");
}

TEST_CASE("縦棒: 全 NaN → 空文字列") {
    CHECK(vbar(std::vector<double>{nan_value, nan_value}) == "");
}

TEST_CASE("横棒: マルチ系列空系列 → 空文字列") {
    CHECK(bar(std::vector<std::vector<double>>{}) == "");
}

TEST_CASE("縦棒: マルチ系列空系列 → 空文字列") {
    CHECK(vbar(std::vector<std::vector<double>>{}) == "");
}

TEST_CASE("横棒: エラー (min > max)") {
    auto cfg = Config{};
    cfg.min = 10.0;
    cfg.max = 1.0;
    CHECK_THROWS_AS(bar(std::vector<double>{1, 2, 3}, cfg), std::invalid_argument);
}

TEST_CASE("縦棒: エラー (min > max)") {
    auto cfg = Config{};
    cfg.min = 10.0;
    cfg.max = 1.0;
    CHECK_THROWS_AS(vbar(std::vector<double>{1, 2, 3}, cfg), std::invalid_argument);
}
