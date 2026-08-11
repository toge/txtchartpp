#!/usr/bin/env python3
"""txtchartpp::bar / txtchartpp::vbar のゴールデンテスト生成スクリプト。

C++ の bar() / vbar() と同じアルゴリズムを Python で実装し、
その出力を test/test_bar.cpp の期待値として埋め込む。
"""

from math import isnan, nan

def utf8(cp):
    if cp < 0x80: return bytes([cp])
    if cp < 0x800: return bytes([0xc0|(cp>>6), 0x80|(cp&0x3f)])
    if cp < 0x10000: return bytes([0xe0|(cp>>12), 0x80|((cp>>6)&0x3f), 0x80|(cp&0x3f)])
    return bytes([0xf0|(cp>>18), 0x80|((cp>>12)&0x3f), 0x80|((cp>>6)&0x3f), 0x80|(cp&0x3f)])

def py_round(x):
    # Python の round() は banker's rounding なのでそのまま使う
    return round(x)

def bar(series, cfg=None):
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

    def bar_len(v):
        if interval <= 0:
            return 0
        return int(py_round(v / interval * height))

    out_lines = []
    for c in range(categories):
        for s in series:
            v = s[c] if c < len(s) else nan
            if isnan(v):
                out_lines.append(' ' * label_width)
                continue
            label = fmt.format(v)
            row = ' ' * (label_width - len(label)) + label
            row += bar_sym * bar_len(v)
            out_lines.append(row)
    return '\n'.join(out_lines)

def vbar(series, cfg=None):
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

    height = cfg.get('height', interval)
    rows = max(int(height), 1)

    def scaled(v):
        if interval <= 0:
            return (rows - 1) // 2
        return int(py_round((maximum - v) / interval * (rows - 1)))

    zero_y = scaled(0.0) if minimum <= 0 <= maximum else -1

    categories = max(len(s) for s in series) if series else 0

    fmt = cfg.get('format', '{:8.2f} ')
    offset = cfg.get('offset', 3)
    symbols = cfg.get('symbols', ['┼', '┤', '╶', '╴', '─', '╰', '╭', '╮', '╯', '│'])
    bar_sym = cfg.get('bar_symbol', '█')

    out_lines = []
    for r in range(rows):
        label_value = maximum - (r * interval / (rows - 1 if rows > 1 else 1))
        label = fmt.format(label_value)
        line = ' ' * max(offset - len(label), 0) + label
        line += symbols[0] if r == zero_y else symbols[1]
        for c in range(categories):
            for s in series:
                if c < len(s) and not isnan(s[c]):
                    y = scaled(s[c])
                    lo = min(y, zero_y if zero_y >= 0 else y)
                    hi = max(y, zero_y if zero_y >= 0 else y)
                    line += bar_sym if lo <= r <= hi else ' '
                else:
                    line += ' '
            if c + 1 < categories:
                line += ' '
        out_lines.append(line)
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

# テストケース
cases = [
    ("bar_simple",  bar([1, 2, 3, 4])),
    ("bar_neg",     bar([-3, -2, -1, 0, 1, 2, 3])),
    ("bar_multi",   bar([[10, 20, 30], [40, 30, 20]], {'height': 10})),
    ("bar_nan",     bar([1, 2, nanv, 4])),
    ("bar_flat",    bar([2.0, 2.0, 2.0])),
    ("vbar_simple", vbar([1, 2, 3, 4])),
    ("vbar_neg",    vbar([-3, -2, -1, 0, 1, 2, 3])),
    ("vbar_multi",  vbar([[10, 20, 30], [40, 30, 20]], {'height': 6})),
    ("vbar_nan",    vbar([1, 2, nanv, 4])),
    ("vbar_flat",   vbar([2.0, 2.0, 2.0])),
]

hdr = '''/**
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

'''

blocks = []
def add_case(name, body, decl, call):
    blocks.append('TEST_CASE("' + name + '") {\n' + body + decl + '    CHECK(' + call + ');\n}\n')

add_case("横棒: 基本",
    '    auto const series = std::vector<double>{1, 2, 3, 4};\n',
    cpp_literal('bar_simple', cases[0][1]),
    'bar(series) == expected_bar_simple')

add_case("横棒: 負値",
    '    auto const series = std::vector<double>{-3, -2, -1, 0, 1, 2, 3};\n',
    cpp_literal('bar_neg', cases[1][1]),
    'bar(series) == expected_bar_neg')

add_case("横棒: 多系列",
    '    auto const series = std::vector<std::vector<double>>{\n'
    '        {10, 20, 30},\n'
    '        {40, 30, 20},\n'
    '    };\n'
    '    auto cfg = Config{};\n'
    '    cfg.height = 10.0;\n',
    cpp_literal('bar_multi', cases[2][1]),
    'bar(series, cfg) == expected_bar_multi')

add_case("横棒: NaN スキップ",
    '    auto const series = std::vector<double>{1, 2, nan_value, 4};\n',
    cpp_literal('bar_nan', cases[3][1]),
    'bar(series) == expected_bar_nan')

add_case("横棒: 一定値",
    '    auto const series = std::vector<double>{2.0, 2.0, 2.0};\n',
    cpp_literal('bar_flat', cases[4][1]),
    'bar(series) == expected_bar_flat')

add_case("縦棒: 基本",
    '    auto const series = std::vector<double>{1, 2, 3, 4};\n',
    cpp_literal('vbar_simple', cases[5][1]),
    'vbar(series) == expected_vbar_simple')

add_case("縦棒: 負値",
    '    auto const series = std::vector<double>{-3, -2, -1, 0, 1, 2, 3};\n',
    cpp_literal('vbar_neg', cases[6][1]),
    'vbar(series) == expected_vbar_neg')

add_case("縦棒: 多系列",
    '    auto const series = std::vector<std::vector<double>>{\n'
    '        {10, 20, 30},\n'
    '        {40, 30, 20},\n'
    '    };\n'
    '    auto cfg = Config{};\n'
    '    cfg.height = 6.0;\n',
    cpp_literal('vbar_multi', cases[7][1]),
    'vbar(series, cfg) == expected_vbar_multi')

add_case("縦棒: NaN スキップ",
    '    auto const series = std::vector<double>{1, 2, nan_value, 4};\n',
    cpp_literal('vbar_nan', cases[8][1]),
    'vbar(series) == expected_vbar_nan')

add_case("縦棒: 一定値",
    '    auto const series = std::vector<double>{2.0, 2.0, 2.0};\n',
    cpp_literal('vbar_flat', cases[9][1]),
    'vbar(series) == expected_vbar_flat')

# エッジケース
add_case("横棒: 空系列 → 空文字列",
    '', '',
    'bar(std::vector<double>{}) == ""')

add_case("横棒: 全 NaN → 空文字列",
    '', '',
    'bar(std::vector<double>{nan_value, nan_value}) == ""')

add_case("縦棒: 空系列 → 空文字列",
    '', '',
    'vbar(std::vector<double>{}) == ""')

add_case("縦棒: 全 NaN → 空文字列",
    '', '',
    'vbar(std::vector<double>{nan_value, nan_value}) == ""')

blocks.append('TEST_CASE("横棒: マルチ系列空系列 → 空文字列") {\n'
    '    CHECK(bar(std::vector<std::vector<double>>{}) == "");\n'
    '}\n')

blocks.append('TEST_CASE("縦棒: マルチ系列空系列 → 空文字列") {\n'
    '    CHECK(vbar(std::vector<std::vector<double>>{}) == "");\n'
    '}\n')

blocks.append('TEST_CASE("横棒: エラー (min > max)") {\n'
    '    auto cfg = Config{};\n'
    '    cfg.min = 10.0;\n'
    '    cfg.max = 1.0;\n'
    '    CHECK_THROWS_AS(bar(std::vector<double>{1, 2, 3}, cfg), std::invalid_argument);\n'
    '}\n')

blocks.append('TEST_CASE("縦棒: エラー (min > max)") {\n'
    '    auto cfg = Config{};\n'
    '    cfg.min = 10.0;\n'
    '    cfg.max = 1.0;\n'
    '    CHECK_THROWS_AS(vbar(std::vector<double>{1, 2, 3}, cfg), std::invalid_argument);\n'
    '}\n')

out = hdr + '\n'.join(blocks)
open('/home/toge/src/txtchartpp/test/test_bar.cpp', 'w').write(out)
print("written", len(out), "bytes")
for name, val in cases:
    print("###", name, "###")
    print(val)
