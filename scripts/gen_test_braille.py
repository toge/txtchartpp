from math import isnan
from fractions import Fraction

def utf8(cp):
    if cp < 0x80: return bytes([cp])
    if cp < 0x800: return bytes([0xc0|(cp>>6), 0x80|(cp&0x3f)])
    if cp < 0x10000: return bytes([0xe0|(cp>>12), 0x80|((cp>>6)&0x3f), 0x80|(cp&0x3f)])
    return bytes([0xf0|(cp>>18), 0x80|((cp>>12)&0x3f), 0x80|((cp>>6)&0x3f), 0x80|(cp&0x3f)])

LEFT_BITS  = [1, 2, 4, 64]
RIGHT_BITS = [8, 16, 32, 128]

def plot_braille(series, cfg=None):
    cfg = cfg or {}
    if len(series) == 0:
        return ''
    if isinstance(series[0], (int, float)):
        if all(isnan(v) for v in series):
            return ''
        series = [series]
    series = [[float(v) for v in s] for s in series]

    vals = [v for s in series for v in s if not isnan(v)]
    if not vals:
        return ''
    minimum = cfg.get('min', min(vals))
    maximum = cfg.get('max', max(vals))
    if minimum > maximum:
        raise ValueError('The min value cannot exceed the max value.')
    interval = maximum - minimum

    cell_rows = int(cfg.get('height', 10))
    cell_rows = max(cell_rows, 1)
    px_rows = cell_rows * 4

    max_len = max(len(s) for s in series) if series else 0
    if max_len == 0:
        return ''
    cell_cols = (max_len + 1) // 2

    def clamp(n):
        return min(max(n, minimum), maximum)

    def py(v):
        if interval <= 0:
            return (px_rows - 1) // 2
        return round((clamp(v) - minimum) / interval * (px_rows - 1))

    cells = [0] * (cell_cols * cell_rows)

    def set_pixel(x, y):
        if y < 0 or y >= px_rows:
            return
        cx = x // 2
        if cx >= cell_cols:
            return
        cy = y // 4
        dy = 3 - (y % 4)  # py%4=0 → 下, py%4=3 → 上 (レンダーの上下反転補正)
        dx = x % 2
        bit = LEFT_BITS[dy] if dx == 0 else RIGHT_BITS[dy]
        cells[cy * cell_cols + cx] |= bit

    for s in series:
        ok = [not isnan(v) for v in s]
        ys = [py(v) if ok[i] else -1 for i, v in enumerate(s)]
        for i in range(len(s)):
            if not ok[i]:
                continue
            set_pixel(i, ys[i])
            if i + 1 < len(s) and ok[i + 1]:
                lo = min(ys[i], ys[i+1])
                hi = max(ys[i], ys[i+1])
                for y in range(lo, hi + 1):
                    set_pixel(i, y)
                set_pixel(i + 1, ys[i+1])

    lines = []
    for cy in range(cell_rows - 1, -1, -1):
        line = ''
        for cx in range(cell_cols):
            b = cells[cy * cell_cols + cx]
            line += chr(0x2800 + b)
        lines.append(line)

    # Y 軸ラベルと軸シンボル
    offset = cfg.get('offset', 3)
    symbols = cfg.get('symbols', ['┼', '┤', '╶', '╴', '─', '╰', '╭', '╮', '╯', '│'])
    fmt = cfg.get('format', '{:8.2f} ')

    # ゼロラインの計算
    zero_line = -1
    if minimum <= 0 <= maximum:
        if interval > 0:
            py_zero = py(0)
            zero_line = cell_rows - 1 - py_zero // 4
        else:
            zero_line = cell_rows // 2

    out_lines = []
    for i in range(cell_rows):
        label_value = maximum - (i * interval / (cell_rows - 1)) if cell_rows > 1 else maximum
        label = fmt.format(label_value)
        axis_symbol = symbols[0] if i == zero_line else symbols[1]
        col = max(offset - len(label), 0)
        left_part = ' ' * col + label
        pad = (offset - 1) - col - len(label)
        if pad > 0:
            left_part += ' ' * pad
        left_part += axis_symbol
        out_lines.append(left_part + lines[i])
    return '\n'.join(out_lines)

def cpp_escape(s):
    out = []
    for ch in s:
        o = ord(ch)
        if ch == '\n':
            out.append('\\n"\n"')
        elif ch == '\\':
            out.append('\\\\')
        elif ch == '"':
            out.append('\\"')
        elif 32 <= o < 127:
            out.append(ch)
        else:
            out.append(''.join('\\x%02x' % b for b in utf8(o)))
    return ''.join(out)

def cpp_literal(name, val):
    return f'    auto const expected_{name} = std::string{{\n    "{cpp_escape(val)}"\n  }};\n'

from math import nan, sin, pi

w = 8
s0 = [round(7*sin(i*((pi*4)/w)), 2) for i in range(w)]
nanv = nan
cases = [
    ("bwave",   plot_braille(s0)),
    ("bwave_h", plot_braille(s0, {'height': 4})),
    ("bramp",   plot_braille([1,2,3,4,5,6,7,8,9])),
    ("bnan",    plot_braille([1,2,3,4,nanv,4,3,2,1])),
    ("bminmax", plot_braille([1,2,3,4,nanv,4,3,2,1], {'min': 0})),
    ("bmulti",  plot_braille([[10,20,30,40,30,20,10],[40,30,20,10,20,30,40]], {'height': 4})),
    ("bflat",   plot_braille([2.0]*8)),
    ("bneg",    plot_braille([-3,-2,-1,0,1,2,3])),
    ("bempty",  plot_braille([])),
    ("ballnan", plot_braille([nanv, nanv])),
    ("bsingle", plot_braille([5])),
]

hdr = '''/**
 * @file test_braille.cpp
 * @brief txtchartpp::plot_braille のゴールデンテスト
 * @author toge (toge.mail@gmail.com)
 * @date 2026-08-09
 * @copyright Copyright (c) 2026 toge(toge.mail@gmail.com)
 *
 * @details
 * 期待値は scripts/gen_test_braille.py の参照実装が生成する。
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

blocks = []
def add_case(name, body, decl, call):
    blocks.append('TEST_CASE("' + name + '") {\n' + body + decl + '    CHECK(' + call + ');\n}\n')

add_case("点字: 波形 (デフォルト高さ)", 
    '    auto const series = std::vector<double>{0, 7, 0, -7, 0, 7, 0, -7};\n',
    cpp_literal('bwave', cases[0][1]),
    'plot_braille(series) == expected_bwave')

add_case("点字: 波形 (height=4)",
    '    auto const series = std::vector<double>{0, 7, 0, -7, 0, 7, 0, -7};\n'
    '    auto cfg = Config{};\n'
    '    cfg.height = 4.0;\n',
    cpp_literal('bwave_h', cases[1][1]),
    'plot_braille(series, cfg) == expected_bwave_h')

add_case("点字: ランプ (1..9)",
    '    auto const series = std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8, 9};\n',
    cpp_literal('bramp', cases[2][1]),
    'plot_braille(series) == expected_bramp')

add_case("点字: NaN スキップ",
    '    auto const series = std::vector<double>{1, 2, 3, 4, nan_value, 4, 3, 2, 1};\n',
    cpp_literal('bnan', cases[3][1]),
    'plot_braille(series) == expected_bnan')

add_case("点字: min 設定 (クリップ)",
    '    auto const series = std::vector<double>{1, 2, 3, 4, nan_value, 4, 3, 2, 1};\n'
    '    auto cfg = Config{};\n'
    '    cfg.min = 0.0;\n',
    cpp_literal('bminmax', cases[4][1]),
    'plot_braille(series, cfg) == expected_bminmax')

add_case("点字: 多系列",
    '    auto const series = std::vector<std::vector<double>>{\n'
    '        {10, 20, 30, 40, 30, 20, 10},\n'
    '        {40, 30, 20, 10, 20, 30, 40},\n'
    '    };\n'
    '    auto cfg = Config{};\n'
    '    cfg.height = 4.0;\n',
    cpp_literal('bmulti', cases[5][1]),
    'plot_braille(series, cfg) == expected_bmulti')

add_case("点字: 一定値",
    '    auto const series = std::vector<double>{2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0};\n',
    cpp_literal('bflat', cases[6][1]),
    'plot_braille(series) == expected_bflat')

add_case("点字: 負値",
    '    auto const series = std::vector<double>{-3, -2, -1, 0, 1, 2, 3};\n',
    cpp_literal('bneg', cases[7][1]),
    'plot_braille(series) == expected_bneg')

add_case("点字: 空系列 → 空文字列",
    '', '',
    'plot_braille(std::vector<double>{}) == ""')

add_case("点字: 全 NaN → 空文字列",
    '', '',
    'plot_braille(std::vector<double>{nan_value, nan_value}) == ""')

add_case("点字: 単一値",
    '    auto const series = std::vector<double>{5};\n',
    cpp_literal('bsingle', cases[10][1]),
    'plot_braille(series) == expected_bsingle')

blocks.append('TEST_CASE("点字: マルチ系列空系列 → 空文字列") {\n'
    '    CHECK(plot_braille(std::vector<std::vector<double>>{}) == "");\n'
    '}\n')

blocks.append('TEST_CASE("点字: マルチ系列全 NaN → 空文字列") {\n'
    '    CHECK(plot_braille(std::vector<std::vector<double>>{{nan_value, nan_value}}) == "");\n'
    '}\n')

blocks.append('TEST_CASE("点字: エラー (min > max)") {\n'
    '    auto cfg = Config{};\n'
    '    cfg.min = 10.0;\n'
    '    cfg.max = 1.0;\n'
    '    CHECK_THROWS_AS(plot_braille(std::vector<double>{1, 2, 3}, cfg), std::invalid_argument);\n'
    '}\n')

import os
p = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'test', 'test_braille.cpp')
open(p, 'w').write(hdr + '\n'.join(blocks))
print("written")
for name, val in cases:
    print("###", name, "###")
    print(val)
