/**
 * @file test_plot.cpp
 * @brief txtchartpp::plot のゴールデンテスト
 * @author toge (toge.mail@gmail.com)
 * @date 2026-08-09
 * @copyright Copyright (c) 2026 toge(toge.mail@gmail.com)
 *
 * @details
 * 期待値は asciichartpy 1.5.25 の plot() の実出力とバイト単位で一致する。
 * Python 側 (scripts/gen_test.py) で生成している。
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

TEST_CASE("単系列の基本描画: 波形 (デフォルト設定)") {
    auto const series = std::vector<double>{0, 7, 0, -7, 0, 7, 0, -7};
    auto const expected_wave = std::string{
    "    7.00  \xe2\x94\xa4\xe2\x95\xad\xe2\x95\xae  \xe2\x95\xad\xe2\x95\xae\n"
"    6.00  \xe2\x94\xa4\xe2\x94\x82\xe2\x94\x82  \xe2\x94\x82\xe2\x94\x82\n"
"    5.00  \xe2\x94\xa4\xe2\x94\x82\xe2\x94\x82  \xe2\x94\x82\xe2\x94\x82\n"
"    4.00  \xe2\x94\xa4\xe2\x94\x82\xe2\x94\x82  \xe2\x94\x82\xe2\x94\x82\n"
"    3.00  \xe2\x94\xa4\xe2\x94\x82\xe2\x94\x82  \xe2\x94\x82\xe2\x94\x82\n"
"    2.00  \xe2\x94\xa4\xe2\x94\x82\xe2\x94\x82  \xe2\x94\x82\xe2\x94\x82\n"
"    1.00  \xe2\x94\xa4\xe2\x94\x82\xe2\x94\x82  \xe2\x94\x82\xe2\x94\x82\n"
"    0.00  \xe2\x94\xbc\xe2\x95\xaf\xe2\x95\xb0\xe2\x95\xae\xe2\x95\xad\xe2\x95\xaf\xe2\x95\xb0\xe2\x95\xae\n"
"   -1.00  \xe2\x94\xa4  \xe2\x94\x82\xe2\x94\x82  \xe2\x94\x82\n"
"   -2.00  \xe2\x94\xa4  \xe2\x94\x82\xe2\x94\x82  \xe2\x94\x82\n"
"   -3.00  \xe2\x94\xa4  \xe2\x94\x82\xe2\x94\x82  \xe2\x94\x82\n"
"   -4.00  \xe2\x94\xa4  \xe2\x94\x82\xe2\x94\x82  \xe2\x94\x82\n"
"   -5.00  \xe2\x94\xa4  \xe2\x94\x82\xe2\x94\x82  \xe2\x94\x82\n"
"   -6.00  \xe2\x94\xa4  \xe2\x94\x82\xe2\x94\x82  \xe2\x94\x82\n"
"   -7.00  \xe2\x94\xa4  \xe2\x95\xb0\xe2\x95\xaf  \xe2\x95\xb0"
  };
    CHECK(plot(series) == expected_wave);
}

TEST_CASE("単系列の基本描画: ランプ (1..9)") {
    auto const series = std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8, 9};
    auto const expected_ramp = std::string{
    "    9.00  \xe2\x94\xa4       \xe2\x95\xad\n"
"    8.00  \xe2\x94\xa4      \xe2\x95\xad\xe2\x95\xaf\n"
"    7.00  \xe2\x94\xa4     \xe2\x95\xad\xe2\x95\xaf\n"
"    6.00  \xe2\x94\xa4    \xe2\x95\xad\xe2\x95\xaf\n"
"    5.00  \xe2\x94\xa4   \xe2\x95\xad\xe2\x95\xaf\n"
"    4.00  \xe2\x94\xa4  \xe2\x95\xad\xe2\x95\xaf\n"
"    3.00  \xe2\x94\xa4 \xe2\x95\xad\xe2\x95\xaf\n"
"    2.00  \xe2\x94\xa4\xe2\x95\xad\xe2\x95\xaf\n"
"    1.00  \xe2\x94\xbc\xe2\x95\xaf"
  };
    CHECK(plot(series) == expected_ramp);
}

TEST_CASE("NaN スキップ: 欠損データ") {
    auto const series = std::vector<double>{1, 2, 3, 4, nan_value, 4, 3, 2, 1};
    auto const expected_nan = std::string{
    "    4.00  \xe2\x94\xa4  \xe2\x95\xad\xe2\x95\xb4\xe2\x95\xb6\xe2\x95\xae\n"
"    3.00  \xe2\x94\xa4 \xe2\x95\xad\xe2\x95\xaf  \xe2\x95\xb0\xe2\x95\xae\n"
"    2.00  \xe2\x94\xa4\xe2\x95\xad\xe2\x95\xaf    \xe2\x95\xb0\xe2\x95\xae\n"
"    1.00  \xe2\x94\xbc\xe2\x95\xaf      \xe2\x95\xb0"
  };
    CHECK(plot(series) == expected_nan);
}

TEST_CASE("min 設定: クリップ") {
    auto const series = std::vector<double>{1, 2, 3, 4, nan_value, 4, 3, 2, 1};
    auto cfg = Config{};
    cfg.min = 0.0;
    auto const expected_min0 = std::string{
    "    4.00  \xe2\x94\xbc  \xe2\x95\xad\xe2\x95\xb4\xe2\x95\xb6\xe2\x95\xae\n"
"    3.00  \xe2\x94\xa4 \xe2\x95\xad\xe2\x95\xaf  \xe2\x95\xb0\xe2\x95\xae\n"
"    2.00  \xe2\x94\xa4\xe2\x95\xad\xe2\x95\xaf    \xe2\x95\xb0\xe2\x95\xae\n"
"    1.00  \xe2\x94\xbc\xe2\x95\xaf      \xe2\x95\xb0\n"
"    0.00  \xe2\x94\xa4"
  };
    CHECK(plot(series, cfg) == expected_min0);
}

TEST_CASE("min 設定: min=2") {
    auto const series = std::vector<double>{1, 2, 3, 4, nan_value, 4, 3, 2, 1};
    auto cfg = Config{};
    cfg.min = 2.0;
    auto const expected_min2 = std::string{
    "    4.00  \xe2\x94\xa4  \xe2\x95\xad\xe2\x95\xb4\xe2\x95\xb6\xe2\x95\xae\n"
"    3.00  \xe2\x94\xa4 \xe2\x95\xad\xe2\x95\xaf  \xe2\x95\xb0\xe2\x95\xae\n"
"    2.00  \xe2\x94\xbc\xe2\x94\x80\xe2\x95\xaf    \xe2\x95\xb0\xe2\x94\x80"
  };
    CHECK(plot(series, cfg) == expected_min2);
}

TEST_CASE("min/max 設定: レンジ狭小") {
    auto const series = std::vector<double>{1, 2, 3, 4, nan_value, 4, 3, 2, 1};
    auto cfg = Config{};
    cfg.min = 2.0;
    cfg.max = 3.0;
    auto const expected_minmax = std::string{
    "    3.00  \xe2\x94\xa4 \xe2\x95\xad\xe2\x94\x80\xe2\x95\xb4\xe2\x95\xb6\xe2\x94\x80\xe2\x95\xae\n"
"    2.00  \xe2\x94\xbc\xe2\x94\x80\xe2\x95\xaf    \xe2\x95\xb0\xe2\x94\x80"
  };
    CHECK(plot(series, cfg) == expected_minmax);
}

TEST_CASE("height 設定: 高さ4") {
    auto const series = std::vector<double>{10, 20, 30, 40, 50, 40, 30, 20, 10};
    auto cfg = Config{};
    cfg.height = 4.0;
    auto const expected_height4 = std::string{
    "   50.00  \xe2\x94\xa4   \xe2\x95\xad\xe2\x95\xae\n"
"   40.00  \xe2\x94\xa4  \xe2\x95\xad\xe2\x95\xaf\xe2\x95\xb0\xe2\x95\xae\n"
"   30.00  \xe2\x94\xa4 \xe2\x95\xad\xe2\x95\xaf  \xe2\x95\xb0\xe2\x95\xae\n"
"   20.00  \xe2\x94\xa4\xe2\x95\xad\xe2\x95\xaf    \xe2\x95\xb0\xe2\x95\xae\n"
"   10.00  \xe2\x94\xbc\xe2\x95\xaf      \xe2\x95\xb0"
  };
    CHECK(plot(series, cfg) == expected_height4);
}

TEST_CASE("format 設定: 小数点なし") {
    auto const series = std::vector<double>{10, 20, 30, 40, 50, 40, 30, 20, 10};
    auto cfg = Config{};
    cfg.height = 4.0;
    cfg.format = "{:8.0f}";
    auto const expected_format0 = std::string{
    "      50 \xe2\x94\xa4   \xe2\x95\xad\xe2\x95\xae\n"
"      40 \xe2\x94\xa4  \xe2\x95\xad\xe2\x95\xaf\xe2\x95\xb0\xe2\x95\xae\n"
"      30 \xe2\x94\xa4 \xe2\x95\xad\xe2\x95\xaf  \xe2\x95\xb0\xe2\x95\xae\n"
"      20 \xe2\x94\xa4\xe2\x95\xad\xe2\x95\xaf    \xe2\x95\xb0\xe2\x95\xae\n"
"      10 \xe2\x94\xbc\xe2\x95\xaf      \xe2\x95\xb0"
  };
    CHECK(plot(series, cfg) == expected_format0);
}

TEST_CASE("offset 設定: 軸オフセット5") {
    auto const series = std::vector<double>{10, 20, 30, 40, 50, 40, 30, 20, 10};
    auto cfg = Config{};
    cfg.height = 4.0;
    cfg.offset = 5;
    auto const expected_offset = std::string{
    "   50.00    \xe2\x94\xa4   \xe2\x95\xad\xe2\x95\xae\n"
"   40.00    \xe2\x94\xa4  \xe2\x95\xad\xe2\x95\xaf\xe2\x95\xb0\xe2\x95\xae\n"
"   30.00    \xe2\x94\xa4 \xe2\x95\xad\xe2\x95\xaf  \xe2\x95\xb0\xe2\x95\xae\n"
"   20.00    \xe2\x94\xa4\xe2\x95\xad\xe2\x95\xaf    \xe2\x95\xb0\xe2\x95\xae\n"
"   10.00    \xe2\x94\xbc\xe2\x95\xaf      \xe2\x95\xb0"
  };
    CHECK(plot(series, cfg) == expected_offset);
}

TEST_CASE("多系列") {
    auto const series = std::vector<std::vector<double>>{
        {10, 20, 30, 40, 30, 20, 10},
        {40, 30, 20, 10, 20, 30, 40},
    };
    auto cfg = Config{};
    cfg.height = 3.0;
    auto const expected_multi = std::string{
    "   40.00  \xe2\x94\xa4\xe2\x95\xae \xe2\x95\xad\xe2\x95\xae \xe2\x95\xad\n"
"   30.00  \xe2\x94\xa4\xe2\x95\xb0\xe2\x95\xae\xe2\x95\xaf\xe2\x95\xb0\xe2\x95\xad\xe2\x95\xaf\n"
"   20.00  \xe2\x94\xa4\xe2\x95\xad\xe2\x95\xb0\xe2\x95\xae\xe2\x95\xad\xe2\x95\xaf\xe2\x95\xae\n"
"   10.00  \xe2\x94\xbc\xe2\x95\xaf \xe2\x95\xb0\xe2\x95\xaf \xe2\x95\xb0"
  };
    CHECK(plot(series, cfg) == expected_multi);
}

TEST_CASE("colors: 2系列2色") {
    auto const series = std::vector<std::vector<double>>{
        {10, 20, 30, 40, 30, 20, 10},
        {40, 30, 20, 10, 20, 30, 40},
    };
    auto cfg = Config{};
    cfg.height = 3.0;
    cfg.colors = {red, blue};
    auto const expected_colors = std::string{
    "   40.00  \xe2\x94\xa4\x1b[34m\xe2\x95\xae\x1b[0m \x1b[31m\xe2\x95\xad\x1b[0m\x1b[31m\xe2\x95\xae\x1b[0m \x1b[34m\xe2\x95\xad\x1b[0m\n"
"   30.00  \xe2\x94\xa4\x1b[34m\xe2\x95\xb0\x1b[0m\x1b[34m\xe2\x95\xae\x1b[0m\x1b[31m\xe2\x95\xaf\x1b[0m\x1b[31m\xe2\x95\xb0\x1b[0m\x1b[34m\xe2\x95\xad\x1b[0m\x1b[34m\xe2\x95\xaf\x1b[0m\n"
"   20.00  \xe2\x94\xa4\x1b[31m\xe2\x95\xad\x1b[0m\x1b[34m\xe2\x95\xb0\x1b[0m\x1b[34m\xe2\x95\xae\x1b[0m\x1b[34m\xe2\x95\xad\x1b[0m\x1b[34m\xe2\x95\xaf\x1b[0m\x1b[31m\xe2\x95\xae\x1b[0m\n"
"   10.00  \xe2\x94\xbc\x1b[31m\xe2\x95\xaf\x1b[0m \x1b[34m\xe2\x95\xb0\x1b[0m\x1b[34m\xe2\x95\xaf\x1b[0m \x1b[31m\xe2\x95\xb0\x1b[0m"
  };
    CHECK(plot(series, cfg) == expected_colors);
}

TEST_CASE("エッジケース: 空系列 → 空文字列") {
    CHECK(plot(std::vector<double>{}) == "");
}

TEST_CASE("エッジケース: 全 NaN → 空文字列") {
    CHECK(plot(std::vector<double>{nan_value, nan_value, nan_value}) == "");
}

TEST_CASE("エッジケース: 一定値") {
    auto const series = std::vector<double>{2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0};
    auto const expected_flat = std::string{
    "    2.00  \xe2\x94\xbc\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
  };
    CHECK(plot(series) == expected_flat);
}

TEST_CASE("エッジケース: 一定値 height 指定") {
    auto const series = std::vector<double>{2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0};
    auto cfg = Config{};
    cfg.height = 5.0;
    auto const expected_flat_h = std::string{
    "    2.00  \xe2\x94\xbc\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
  };
    CHECK(plot(series, cfg) == expected_flat_h);
}

TEST_CASE("エッジケース: 負値") {
    auto const series = std::vector<double>{-3, -2, -1, 0, 1, 2, 3};
    auto const expected_neg = std::string{
    "    3.00  \xe2\x94\xa4     \xe2\x95\xad\n"
"    2.00  \xe2\x94\xa4    \xe2\x95\xad\xe2\x95\xaf\n"
"    1.00  \xe2\x94\xa4   \xe2\x95\xad\xe2\x95\xaf\n"
"    0.00  \xe2\x94\xbc  \xe2\x95\xad\xe2\x95\xaf\n"
"   -1.00  \xe2\x94\xa4 \xe2\x95\xad\xe2\x95\xaf\n"
"   -2.00  \xe2\x94\xa4\xe2\x95\xad\xe2\x95\xaf\n"
"   -3.00  \xe2\x94\xbc\xe2\x95\xaf"
  };
    CHECK(plot(series) == expected_neg);
}

TEST_CASE("エラー: min > max → std::invalid_argument") {
    auto cfg = Config{};
    cfg.min = 10.0;
    cfg.max = 1.0;
    CHECK_THROWS_AS(plot(std::vector<double>{1, 2, 3}, cfg), std::invalid_argument);
}
