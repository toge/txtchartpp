from asciichartpy import plot, red, green, blue
from math import nan, sin, pi

def utf8(o):
    if o < 0x80: return bytes([o])
    elif o < 0x800: return bytes([0xc0|(o>>6), 0x80|(o&0x3f)])
    elif o < 0x10000: return bytes([0xe0|(o>>12), 0x80|((o>>6)&0x3f), 0x80|(o&0x3f)])
    else: return bytes([0xf0|(o>>18), 0x80|((o>>12)&0x3f), 0x80|((o>>6)&0x3f), 0x80|(o&0x3f)])

def cpp_str(s):
    out = []
    for ch in s:
        o = ord(ch)
        if ch == '\n': out.append('\\n"\n"')
        elif ch == '\\': out.append('\\\\')
        elif ch == '"': out.append('\\"')
        elif 32 <= o < 127: out.append(ch)
        else: out.append(''.join('\\x%02x' % b for b in utf8(o)))
    return ''.join(out)

def cpp_literal(name, val):
    return f'    auto const expected_{name} = std::string{{\n    "{cpp_str(val)}"\n  }};\n'

w = 8
s0 = [round(7*sin(i*((pi*4)/w)), 2) for i in range(w)]
nanv = nan
s = [1,2,3,4,nanv,4,3,2,1]
r = [10,20,30,40,50,40,30,20,10]

cases = [
    ("wave",  plot(s0)),
    ("ramp",  plot([1,2,3,4,5,6,7,8,9])),
    ("nan",   plot(s)),
    ("min0",  plot(s, {'min': 0})),
    ("min2",  plot(s, {'min': 2})),
    ("minmax",plot(s, {'min': 2, 'max': 3})),
    ("height4",plot(r, {'height': 4})),
    ("format0",plot(r, {'height': 4, 'format': '{:8.0f}'})),
    ("offset", plot(r, {'height': 4, 'offset': 5})),
    ("multi", plot([[10,20,30,40,30,20,10],[40,30,20,10,20,30,40]], {'height': 3})),
    ("colors",plot([[10,20,30,40,30,20,10],[40,30,20,10,20,30,40]], {'height': 3, 'colors': [red, blue]})),
    ("flat",  plot([2.0]*8)),
    ("flat_h",plot([2.0]*8, {'height': 5})),
    ("neg",   plot([-3,-2,-1,0,1,2,3])),
]

hdr = '''/**
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

'''

tests = []

tests.append(('''TEST_CASE("単系列の基本描画: 波形 (デフォルト設定)") {
    auto const series = std::vector<double>{0, 7, 0, -7, 0, 7, 0, -7};
''' + cpp_literal('wave', cases[0][1]) + '''    CHECK(plot(series) == expected_wave);
}
'''))

tests.append(('''TEST_CASE("単系列の基本描画: ランプ (1..9)") {
    auto const series = std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8, 9};
''' + cpp_literal('ramp', cases[1][1]) + '''    CHECK(plot(series) == expected_ramp);
}
'''))

tests.append(('''TEST_CASE("NaN スキップ: 欠損データ") {
    auto const series = std::vector<double>{1, 2, 3, 4, nan_value, 4, 3, 2, 1};
''' + cpp_literal('nan', cases[2][1]) + '''    CHECK(plot(series) == expected_nan);
}
'''))

tests.append(('''TEST_CASE("min 設定: クリップ") {
    auto const series = std::vector<double>{1, 2, 3, 4, nan_value, 4, 3, 2, 1};
    auto cfg = Config{};
    cfg.min = 0.0;
''' + cpp_literal('min0', cases[3][1]) + '''    CHECK(plot(series, cfg) == expected_min0);
}
'''))

tests.append(('''TEST_CASE("min 設定: min=2") {
    auto const series = std::vector<double>{1, 2, 3, 4, nan_value, 4, 3, 2, 1};
    auto cfg = Config{};
    cfg.min = 2.0;
''' + cpp_literal('min2', cases[4][1]) + '''    CHECK(plot(series, cfg) == expected_min2);
}
'''))

tests.append(('''TEST_CASE("min/max 設定: レンジ狭小") {
    auto const series = std::vector<double>{1, 2, 3, 4, nan_value, 4, 3, 2, 1};
    auto cfg = Config{};
    cfg.min = 2.0;
    cfg.max = 3.0;
''' + cpp_literal('minmax', cases[5][1]) + '''    CHECK(plot(series, cfg) == expected_minmax);
}
'''))

tests.append(('''TEST_CASE("height 設定: 高さ4") {
    auto const series = std::vector<double>{10, 20, 30, 40, 50, 40, 30, 20, 10};
    auto cfg = Config{};
    cfg.height = 4.0;
''' + cpp_literal('height4', cases[6][1]) + '''    CHECK(plot(series, cfg) == expected_height4);
}
'''))

tests.append(('''TEST_CASE("format 設定: 小数点なし") {
    auto const series = std::vector<double>{10, 20, 30, 40, 50, 40, 30, 20, 10};
    auto cfg = Config{};
    cfg.height = 4.0;
    cfg.format = "{:8.0f}";
''' + cpp_literal('format0', cases[7][1]) + '''    CHECK(plot(series, cfg) == expected_format0);
}
'''))

tests.append(('''TEST_CASE("offset 設定: 軸オフセット5") {
    auto const series = std::vector<double>{10, 20, 30, 40, 50, 40, 30, 20, 10};
    auto cfg = Config{};
    cfg.height = 4.0;
    cfg.offset = 5;
''' + cpp_literal('offset', cases[8][1]) + '''    CHECK(plot(series, cfg) == expected_offset);
}
'''))

tests.append(('''TEST_CASE("多系列") {
    auto const series = std::vector<std::vector<double>>{
        {10, 20, 30, 40, 30, 20, 10},
        {40, 30, 20, 10, 20, 30, 40},
    };
    auto cfg = Config{};
    cfg.height = 3.0;
''' + cpp_literal('multi', cases[9][1]) + '''    CHECK(plot(series, cfg) == expected_multi);
}
'''))

tests.append(('''TEST_CASE("colors: 2系列2色") {
    auto const series = std::vector<std::vector<double>>{
        {10, 20, 30, 40, 30, 20, 10},
        {40, 30, 20, 10, 20, 30, 40},
    };
    auto cfg = Config{};
    cfg.height = 3.0;
    cfg.colors = {red, blue};
''' + cpp_literal('colors', cases[10][1]) + '''    CHECK(plot(series, cfg) == expected_colors);
}
'''))

tests.append(('''TEST_CASE("エッジケース: 空系列 → 空文字列") {
    CHECK(plot(std::vector<double>{}) == "");
}
'''))

tests.append(('''TEST_CASE("エッジケース: 全 NaN → 空文字列") {
    CHECK(plot(std::vector<double>{nan_value, nan_value, nan_value}) == "");
}
'''))

tests.append(('''TEST_CASE("エッジケース: 一定値") {
    auto const series = std::vector<double>{2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0};
''' + cpp_literal('flat', cases[11][1]) + '''    CHECK(plot(series) == expected_flat);
}
'''))

tests.append(('''TEST_CASE("エッジケース: 一定値 height 指定") {
    auto const series = std::vector<double>{2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0};
    auto cfg = Config{};
    cfg.height = 5.0;
''' + cpp_literal('flat_h', cases[12][1]) + '''    CHECK(plot(series, cfg) == expected_flat_h);
}
'''))

tests.append(('''TEST_CASE("エッジケース: 負値") {
    auto const series = std::vector<double>{-3, -2, -1, 0, 1, 2, 3};
''' + cpp_literal('neg', cases[13][1]) + '''    CHECK(plot(series) == expected_neg);
}
'''))

tests.append(('''TEST_CASE("エッジケース: マルチ系列空系列 → 空文字列") {
    CHECK(plot(std::vector<std::vector<double>>{}) == "");
}
'''))

tests.append(('''TEST_CASE("エッジケース: マルチ系列全 NaN → 空文字列") {
    CHECK(plot(std::vector<std::vector<double>>{{nan_value, nan_value}}) == "");
}
'''))

tests.append(('''TEST_CASE("エラー: min > max → std::invalid_argument") {
    auto cfg = Config{};
    cfg.min = 10.0;
    cfg.max = 1.0;
    CHECK_THROWS_AS(plot(std::vector<double>{1, 2, 3}, cfg), std::invalid_argument);
}
'''))

out = hdr + '\n'.join(tests)
import os
p = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'test', 'test_plot.cpp')
open(p, 'w').write(out)
print("written", len(out), "bytes")
