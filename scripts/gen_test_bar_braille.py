#!/usr/bin/env python3
"""txtchartpp::bar_braille / txtchartpp::vbar_braille のゴールデンテスト生成スクリプト。

C++ の bar_braille() / vbar_braille() と同じアルゴリズムを Python で実装し、
その出力を test/test_bar_braille.cpp の期待値として埋め込む。
"""

from math import isnan, nan

def utf8(cp):
    if cp < 0x80: return bytes([cp])
    if cp < 0x800: return bytes([0xc0|(cp>>6), 0x80|(cp&0x3f)])
    if cp < 0x10000: return bytes([0xe0|(cp>>12), 0x80|((cp>>6)&0x3f), 0x80|(cp&0x3f)])
    return bytes([0xf0|(cp>>18), 0x80|((cp>>12)&0x3f), 0x80|((cp>>6)&0x3f), 0x80|(cp&0x3f)])

def py_round(x):
    return round(x)

LEFT_BITS  = [1, 2, 4, 64]
RIGHT_BITS = [8, 16, 32, 128]

def bar_braille(series, cfg=None):
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

    categories = max(len(s) for s in series) if series else 0

    fmt = cfg.get('format', '{:8.2f} ')
    offset = cfg.get('offset', 3)
    bar_sym = cfg.get('bar_symbol', '█')
    height = cfg.get('height', interval)

    # ラベルの最大幅
    label_width = 0
    for s in series:
        for v in s:
            if not isnan(v):
                label_width = max(label_width, len(fmt.format(v)))
    label_width += offset

    def half_len(v):
        if interval <= 0:
            return 0
        return int(py_round(v / interval * height * 2.0))

    out_lines = []
    for c in range(categories):
        for s in series:
            v = s[c] if c < len(s) else nan
            if isnan(v):
                out_lines.append(' ' * label_width)
                continue
            label = fmt.format(v)
            row = ' ' * (label_width - len(label)) + label
            n = half_len(v)
            row += bar_sym * (n // 2)
            if n % 2:
                row += '▌'  # 左半分のブロック
            out_lines.append(row)
    return '\n'.join(out_lines)


def vbar_braille(series, cfg=None):
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

    cell_rows = max(int(cfg.get('height', 10)), 1)
    px_rows = cell_rows * 4

    def scaled(v):
        if interval <= 0:
            return (px_rows - 1) // 2
        return int(py_round((maximum - v) / interval * (px_rows - 1)))

    zero_py = scaled(0.0) if minimum <= 0 <= maximum else -1

    categories = max(len(s) for s in series) if series else 0
    cell_cols = (categories * (len(series) * 2 + 1) + 1) // 2

    cells = [0] * (cell_cols * cell_rows)

    def set_pixel(x, y):
        if y < 0 or y >= px_rows:
            return
        cx = x // 2
        if cx >= cell_cols:
            return
        cy = y // 4
        dy = 3 - (y % 4)
        dx = x % 2
        bit = LEFT_BITS[dy] if dx == 0 else RIGHT_BITS[dy]
        cells[cy * cell_cols + cx] |= bit

    for c in range(categories):
        for si, s in enumerate(series):
            if c >= len(s) or isnan(s[c]):
                continue
            py = scaled(s[c])
            lo = min(py, zero_py if zero_py >= 0 else py)
            hi = max(py, zero_py if zero_py >= 0 else py)
            x0 = c * (len(series) * 2 + 1) + si * 2
            for y in range(lo, hi + 1):
                set_pixel(x0, y)
                set_pixel(x0 + 1, y)

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

    zero_line = -1
    if minimum <= 0 <= maximum:
        if interval > 0:
            zero_line = cell_rows - 1 - scaled(0.0) // 4
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

nanv = nan

cases = [
    ("bbar_simple", bar_braille([1, 2, 3, 4])),
    ("bbar_neg",    bar_braille([-3, -2, -1, 0, 1, 2, 3])),
    ("bbar_multi",  bar_braille([[10, 20, 30], [40, 30, 20]], {'height': 10})),
    ("bbar_nan",    bar_braille([1, 2, nanv, 4])),
    ("bbar_flat",   bar_braille([2.0, 2.0, 2.0])),
    ("bvbar_simple", vbar_braille([1, 2, 3, 4])),
    ("bvbar_neg",    vbar_braille([-3, -2, -1, 0, 1, 2, 3])),
    ("bvbar_multi",  vbar_braille([[10, 20, 30], [40, 30, 20]], {'height': 4})),
    ("bvbar_nan",    vbar_braille([1, 2, nanv, 4])),
    ("bvbar_flat",   vbar_braille([2.0, 2.0, 2.0])),
]

hdr = '''/**
 * @file test_bar_braille.cpp
 * @brief txtchartpp::bar_braille / txtchartpp::vbar_braille のゴールデンテスト
 * @author toge (toge.mail@gmail.com)
 * @date 2026-08-11
 * @copyright Copyright (c) 2026 toge(toge.mail@gmail.com)
 *
 * @details
 * 期待値は scripts/gen_test_bar_braille.py の参照実装が生成する。
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

add_case("点字横棒: 基本",
    '    auto const series = std::vector<double>{1, 2, 3, 4};\n',
    cpp_literal('bbar_simple', cases[0][1]),
    'bar_braille(series) == expected_bbar_simple')

add_case("点字横棒: 負値",
    '    auto const series = std::vector<double>{-3, -2, -1, 0, 1, 2, 3};\n',
    cpp_literal('bbar_neg', cases[1][1]),
    'bar_braille(series) == expected_bbar_neg')

add_case("点字横棒: 多系列",
    '    auto const series = std::vector<std::vector<double>>{\n'
    '        {10, 20, 30},\n'
    '        {40, 30, 20},\n'
    '    };\n'
    '    auto cfg = Config{};\n'
    '    cfg.height = 10.0;\n',
    cpp_literal('bbar_multi', cases[2][1]),
    'bar_braille(series, cfg) == expected_bbar_multi')

add_case("点字横棒: NaN スキップ",
    '    auto const series = std::vector<double>{1, 2, nan_value, 4};\n',
    cpp_literal('bbar_nan', cases[3][1]),
    'bar_braille(series) == expected_bbar_nan')

add_case("点字横棒: 一定値",
    '    auto const series = std::vector<double>{2.0, 2.0, 2.0};\n',
    cpp_literal('bbar_flat', cases[4][1]),
    'bar_braille(series) == expected_bbar_flat')

add_case("点字縦棒: 基本",
    '    auto const series = std::vector<double>{1, 2, 3, 4};\n',
    cpp_literal('bvbar_simple', cases[5][1]),
    'vbar_braille(series) == expected_bvbar_simple')

add_case("点字縦棒: 負値",
    '    auto const series = std::vector<double>{-3, -2, -1, 0, 1, 2, 3};\n',
    cpp_literal('bvbar_neg', cases[6][1]),
    'vbar_braille(series) == expected_bvbar_neg')

add_case("点字縦棒: 多系列",
    '    auto const series = std::vector<std::vector<double>>{\n'
    '        {10, 20, 30},\n'
    '        {40, 30, 20},\n'
    '    };\n'
    '    auto cfg = Config{};\n'
    '    cfg.height = 4.0;\n',
    cpp_literal('bvbar_multi', cases[7][1]),
    'vbar_braille(series, cfg) == expected_bvbar_multi')

add_case("点字縦棒: NaN スキップ",
    '    auto const series = std::vector<double>{1, 2, nan_value, 4};\n',
    cpp_literal('bvbar_nan', cases[8][1]),
    'vbar_braille(series) == expected_bvbar_nan')

add_case("点字縦棒: 一定値",
    '    auto const series = std::vector<double>{2.0, 2.0, 2.0};\n',
    cpp_literal('bvbar_flat', cases[9][1]),
    'vbar_braille(series) == expected_bvbar_flat')

add_case("点字横棒: 空系列 → 空文字列",
    '', '',
    'bar_braille(std::vector<double>{}) == ""')

add_case("点字横棒: 全 NaN → 空文字列",
    '', '',
    'bar_braille(std::vector<double>{nan_value, nan_value}) == ""')

add_case("点字縦棒: 空系列 → 空文字列",
    '', '',
    'vbar_braille(std::vector<double>{}) == ""')

add_case("点字縦棒: 全 NaN → 空文字列",
    '', '',
    'vbar_braille(std::vector<double>{nan_value, nan_value}) == ""')

blocks.append('TEST_CASE("点字横棒: マルチ系列空系列 → 空文字列") {\n'
    '    CHECK(bar_braille(std::vector<std::vector<double>>{}) == "");\n'
    '}\n')

blocks.append('TEST_CASE("点字縦棒: マルチ系列空系列 → 空文字列") {\n'
    '    CHECK(vbar_braille(std::vector<std::vector<double>>{}) == "");\n'
    '}\n')

blocks.append('TEST_CASE("点字横棒: エラー (min > max)") {\n'
    '    auto cfg = Config{};\n'
    '    cfg.min = 10.0;\n'
    '    cfg.max = 1.0;\n'
    '    CHECK_THROWS_AS(bar_braille(std::vector<double>{1, 2, 3}, cfg), std::invalid_argument);\n'
    '}\n')

blocks.append('TEST_CASE("点字縦棒: エラー (min > max)") {\n'
    '    auto cfg = Config{};\n'
    '    cfg.min = 10.0;\n'
    '    cfg.max = 1.0;\n'
    '    CHECK_THROWS_AS(vbar_braille(std::vector<double>{1, 2, 3}, cfg), std::invalid_argument);\n'
    '}\n')

out = hdr + '\n'.join(blocks)
open('/home/toge/src/txtchartpp/test/test_bar_braille.cpp', 'w').write(out)
print("written", len(out), "bytes")
for name, val in cases:
    print("###", name, "###")
    print(val)
