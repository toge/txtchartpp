/**
 * @file txtchart.hpp
 * @brief asciichartpy 互換の ASCII 折れ線グラフ描画ライブラリ (txtchartpp)
 *
 * @author toge (toge.mail@gmail.com)
 * @date 2026-08-09
 *
 * @copyright Copyright (c) 2026 toge(toge.mail@gmail.com)
 *
 * @details
 * Python の asciichartpy 1.5.25 と同一のアルゴリズムでグラフを描画する。
 * 出力文字列は asciichartpy の plot() とバイト単位で一致する。
 */

#ifndef TXTCHARTPP_TXT_CHART_HPP
#define TXTCHARTPP_TXT_CHART_HPP

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <format>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace txtchart {

/** @brief ANSI 色コード定数 (asciichartpy のモジュール定数と互換) */
inline constexpr std::string_view black      = "\033[30m";
inline constexpr std::string_view red        = "\033[31m";
inline constexpr std::string_view green      = "\033[32m";
inline constexpr std::string_view yellow     = "\033[33m";
inline constexpr std::string_view blue       = "\033[34m";
inline constexpr std::string_view magenta    = "\033[35m";
inline constexpr std::string_view cyan       = "\033[36m";
inline constexpr std::string_view lightgray  = "\033[37m";
inline constexpr std::string_view default_   = "\033[39m";
inline constexpr std::string_view darkgray   = "\033[90m";
inline constexpr std::string_view lightred   = "\033[91m";
inline constexpr std::string_view lightgreen = "\033[92m";
inline constexpr std::string_view lightyellow = "\033[93m";
inline constexpr std::string_view lightblue  = "\033[94m";
inline constexpr std::string_view lightmagenta = "\033[95m";
inline constexpr std::string_view lightcyan  = "\033[96m";
inline constexpr std::string_view white      = "\033[97m";
inline constexpr std::string_view reset      = "\033[0m";

/** @brief グラフ描画設定 */
struct Config {
    /** @brief Y 軸の最小値。未設定ならデータの最小値 */
    std::optional<double> min;
    /** @brief Y 軸の最大値。未設定ならデータの最大値 */
    std::optional<double> max;
    /** @brief グラフの高さ (行数)。未設定なら interval (= max - min) */
    std::optional<double> height;
    /** @brief Y 軸ラベルの左マージン (最小 2) */
    int offset = 3;
    /** @brief Y 軸ラベルの書式。std::format の書式文字列 */
    std::string format = "{:8.2f} ";
    /**
     * @brief 描画シンボル (asciichartpy の default_symbols と互換)
     * @details
     * [0] cross  [1] right tee  [2] 左端   [3] 右端   [4] 水平
     * [5] 右上折れ  [6] 右下折れ  [7] 左下折れ  [8] 左上折れ  [9] 垂直
     */
    std::array<std::string, 10> symbols = {
        "┼", "┤", "╶", "╴", "─", "╰", "╭", "╮", "╯", "│",
    };
    /** @brief 系列ごとの ANSI 色コード。空なら色なし (asciichartpy の colors と互換) */
    std::vector<std::string_view> colors;
};

namespace detail {

/** @brief 数値判定。NaN なら false */
inline auto is_number(double const n) noexcept -> bool {
    return !std::isnan(n);
}

/**
 * @brief Python の round() と互換の丸め (banker's rounding)
 * @details Python 3 の round() は round-half-to-even。std::round は
 * half-away-from-zero なので、境界値で出力がずれるのを防ぐ。
 */
inline auto py_round(double const x) -> double {
    double const r = std::round(x);
    double const fr = x - std::floor(x);
    if (std::abs(fr - 0.5) > 1e-12) {
        return r;
    }
    double const f = std::floor(x);
    return std::fmod(f, 2.0) != 0.0 ? f + 1.0 : f;
}

/** @brief 全体の最小値/最大値を求める (NaN は無視) */
inline auto min_max(std::vector<std::vector<double>> const& series)
    -> std::pair<double, double> {
    double minimum = std::numeric_limits<double>::max();
    double maximum = std::numeric_limits<double>::lowest();
    for (auto const& s : series) {
        for (double const v : s) {
            if (!is_number(v)) {
                continue;
            }
            minimum = std::min(minimum, v);
            maximum = std::max(maximum, v);
        }
    }
    return {minimum, maximum};
}

/**
 * @brief コードポイントを UTF-8 バイト列にエンコードする
 */
inline auto utf8_encode(char32_t const cp) -> std::string {
    if (cp < 0x80) {
        return std::string(1, static_cast<char>(cp));
    }
    if (cp < 0x800) {
        return std::string{
            static_cast<char>(0xC0 | (cp >> 6)),
            static_cast<char>(0x80 | (cp & 0x3F)),
        };
    }
    if (cp < 0x10000) {
        return std::string{
            static_cast<char>(0xE0 | (cp >> 12)),
            static_cast<char>(0x80 | ((cp >> 6) & 0x3F)),
            static_cast<char>(0x80 | (cp & 0x3F)),
        };
    }
    return std::string{
        static_cast<char>(0xF0 | (cp >> 18)),
        static_cast<char>(0x80 | ((cp >> 12) & 0x3F)),
        static_cast<char>(0x80 | ((cp >> 6) & 0x3F)),
        static_cast<char>(0x80 | (cp & 0x3F)),
    };
}

/** @brief Braille セル内のドット位置 (0=左上, 1=左2, 2=左3, 3=左4, 4=右上, 5=右2, 6=右3, 7=右4) → ビット */
constexpr int braille_dot_bits[8] = {
    0x01, 0x02, 0x04, 0x40,
    0x08, 0x10, 0x20, 0x80,
};

/** @brief 1セル=2列×4行のピクセルグリッドでドットを立てる */
struct BrailleGrid {
    int cell_rows = 0;
    int cell_cols = 0;
    std::vector<unsigned char> cells;

    BrailleGrid(int const rows, int const cols)
        : cell_rows(rows), cell_cols(cols), cells(static_cast<std::size_t>(rows * cols), 0) {}

    /** @brief ピクセル座標 (px, py) にドットを立てる。範囲外は無視 */
    void set_pixel(int const px, int const py) {
        if (py < 0 || py >= cell_rows * 4) {
            return;
        }
        int const cx = px / 2;
        if (cx >= cell_cols) {
            return;
        }
        int const cy = py / 4;
        int const dot = (3 - (py % 4)) + (px % 2) * 4; // 左列 0-3, 右列 4-7 (上下反転補正)
        cells[static_cast<std::size_t>(cy) * cell_cols + cx] |= braille_dot_bits[dot];
    }

    /** @brief セルグリッドを UTF-8 の Braille 文字列に組み立てる (上から下) */
    auto render() const -> std::string {
        std::string out;
        for (int cy = cell_rows - 1; cy >= 0; --cy) {
            for (int cx = 0; cx < cell_cols; ++cx) {
                unsigned char const b = cells[static_cast<std::size_t>(cy) * cell_cols + cx];
                out += utf8_encode(static_cast<char32_t>(0x2800 + b));
            }
            if (cy != 0) {
                out += '\n';
            }
        }
        return out;
    }
};

} // namespace detail

/**
 * @brief 複数系列を ASCII グラフに描画する
 * @param series 系列のリスト (系列ごとに値の列)
 * @param cfg    描画設定
 * @return グラフ文字列 (asciichartpy の plot() と同一出力)
 * @throws std::invalid_argument min が max より大きい場合
 */
inline auto plot(std::vector<std::vector<double>> const& series, Config const& cfg = {})
    -> std::string {
    // シンボル定義 (asciichartpy の default_symbols と同一)
    // [0]=cross [1]=right tee [2]=左向き [3]=右向き [4]=水平
    // [5]=下がり [6]=上がり [7]=上 [8]=下 [9]=垂直
    auto const& symbols = cfg.symbols;

    // NaN を無視した全体の最小/最大
    auto const [data_min, data_max] = detail::min_max(series);
    double const minimum = cfg.min.value_or(data_min);
    double const maximum = cfg.max.value_or(data_max);

    if (minimum > maximum) {
        throw std::invalid_argument("The min value cannot exceed the max value.");
    }

    double const interval = maximum - minimum;
    int const offset = cfg.offset;
    double const height = cfg.height.value_or(interval);
    double const ratio = interval > 0.0 ? height / interval : 1.0;

    int const min2 = static_cast<int>(std::floor(minimum * ratio));
    int const max2 = static_cast<int>(std::ceil(maximum * ratio));
    int const rows = max2 - min2;

    auto const clamp = [minimum, maximum](double const n) -> double {
        return std::min(std::max(n, minimum), maximum);
    };
    auto const scaled = [ratio, min2, &clamp](double const y) -> int {
        return static_cast<int>(detail::py_round(clamp(y) * ratio) - min2);
    };

    // 幅 = offset + 最長系列の長さ
    std::size_t width = offset;
    for (auto const& s : series) {
        width = std::max(width, static_cast<std::size_t>(offset + static_cast<int>(s.size())));
    }

    // 行の初期化
    std::vector<std::vector<std::string>> result(static_cast<std::size_t>(rows + 1),
        std::vector<std::string>(width, " "));

    // Y 軸ラベルと軸シンボル
    for (int y = min2; y <= max2; ++y) {
        double const label_value = maximum - ((y - min2) * interval / (rows != 0 ? rows : 1));
        std::string const label = std::vformat(cfg.format, std::make_format_args(label_value));
        std::size_t const col = static_cast<std::size_t>(
            std::max(offset - static_cast<int>(label.size()), 0));
        result[static_cast<std::size_t>(y - min2)][col] = label;
        result[static_cast<std::size_t>(y - min2)][static_cast<std::size_t>(offset - 1)] =
            (y == 0) ? symbols[0] : symbols[1];
    }

    // 最初の値は Y 軸上に印を付ける
    double const d0 = series[0][0];
    if (detail::is_number(d0)) {
        result[static_cast<std::size_t>(rows - scaled(d0))][static_cast<std::size_t>(offset - 1)] =
            symbols[0];
    }

    // 各系列を描画
    for (std::size_t i = 0; i < series.size(); ++i) {
        std::string_view const color = cfg.colors.empty() ? std::string_view{}
                                                          : cfg.colors[i % cfg.colors.size()];
        auto const colored = [color](std::string const& ch) -> std::string {
            if (color.empty()) {
                return ch;
            }
            return std::string(color) + ch + std::string(reset);
        };

        auto const& s = series[i];
        for (std::size_t x = 0; x + 1 < s.size(); ++x) {
            double const v0 = s[x];
            double const v1 = s[x + 1];

            if (!detail::is_number(v0) && !detail::is_number(v1)) {
                continue;
            }
            if (!detail::is_number(v0)) {
                result[static_cast<std::size_t>(rows - scaled(v1))][x + static_cast<std::size_t>(offset)] =
                    colored(symbols[2]);
                continue;
            }
            if (!detail::is_number(v1)) {
                result[static_cast<std::size_t>(rows - scaled(v0))][x + static_cast<std::size_t>(offset)] =
                    colored(symbols[3]);
                continue;
            }

            int const y0 = scaled(v0);
            int const y1 = scaled(v1);

            if (y0 == y1) {
                result[static_cast<std::size_t>(rows - y0)][x + static_cast<std::size_t>(offset)] =
                    colored(symbols[4]);
                continue;
            }

            result[static_cast<std::size_t>(rows - y1)][x + static_cast<std::size_t>(offset)] =
                (y0 > y1) ? colored(symbols[5]) : colored(symbols[6]);
            result[static_cast<std::size_t>(rows - y0)][x + static_cast<std::size_t>(offset)] =
                (y0 > y1) ? colored(symbols[7]) : colored(symbols[8]);

            int const start = std::min(y0, y1) + 1;
            int const end = std::max(y0, y1);
            for (int y = start; y < end; ++y) {
                result[static_cast<std::size_t>(rows - y)][x + static_cast<std::size_t>(offset)] =
                    colored(symbols[9]);
            }
        }
    }

    // 各行を結合し、末尾の空白を除去する
    std::string out;
    for (std::size_t r = 0; r < result.size(); ++r) {
        std::string line;
        for (auto const& cell : result[r]) {
            line += cell;
        }
        while (!line.empty() && line.back() == ' ') {
            line.pop_back();
        }
        if (r != 0) {
            out += '\n';
        }
        out += line;
    }
    return out;
}

/**
 * @brief 単一系列を ASCII グラフに描画する
 * @param series 値の列 (NaN で欠損を表せる)
 * @param cfg    描画設定
 * @return グラフ文字列。空系列や全 NaN なら空文字列
 * @throws std::invalid_argument min が max より大きい場合
 */
inline auto plot(std::vector<double> const& series, Config const& cfg = {}) -> std::string {
    if (series.empty()) {
        return {};
    }
    bool all_nan = true;
    for (double const v : series) {
        if (detail::is_number(v)) {
            all_nan = false;
            break;
        }
    }
    if (all_nan) {
        return {};
    }
    return plot(std::vector<std::vector<double>>{series}, cfg);
}

/**
 * @brief 複数系列を Braille 文字で描画する
 * @details
 * Braille 文字は 1 文字あたり 2列×4行のドットを持ち、ASCII 版より高解像度。
 * 系列の各点をピクセル座標に写像し、隣接点を直線で結ぶ。
 * @param series 系列のリスト (NaN はスキップされる)
 * @param cfg    描画設定 (min/max/height のみ使用。offset/format/colors は無視)
 * @return グラフ文字列。空系列や全 NaN なら空文字列
 * @throws std::invalid_argument min が max より大きい場合
 */
inline auto plot_braille(std::vector<std::vector<double>> const& series, Config const& cfg = {})
    -> std::string {
    if (series.empty()) {
        return {};
    }

    // NaN を無視した全体の最小/最大
    auto const [data_min, data_max] = detail::min_max(series);
    if (!detail::is_number(data_min) || !detail::is_number(data_max)) {
        return {};
    }
    double const minimum = cfg.min.value_or(data_min);
    double const maximum = cfg.max.value_or(data_max);
    if (minimum > maximum) {
        throw std::invalid_argument("The min value cannot exceed the max value.");
    }
    double const interval = maximum - minimum;

    // 高さはセル行数。各セルが 4 ピクセル行に相当する
    int const cell_rows = std::max(static_cast<int>(std::lround(cfg.height.value_or(10.0))), 1);
    int const px_rows = cell_rows * 4;

    // 幅は最長系列の長さ。各データ点が 1 ピクセル列 = 0.5 セル
    std::size_t max_len = 0;
    for (auto const& s : series) {
        max_len = std::max(max_len, s.size());
    }
    if (max_len == 0) {
        return {};
    }
    int const cell_cols = static_cast<int>((max_len + 1) / 2);

    auto const clamp = [minimum, maximum](double const n) -> double {
        return std::min(std::max(n, minimum), maximum);
    };
    // 値 → ピクセル行 (0 = 最大値, px_rows-1 = 最小値)
    auto const scaled = [&](double const v) -> int {
        if (interval <= 0.0) {
            return (px_rows - 1) / 2;
        }
        double const p = detail::py_round((clamp(v) - minimum) / interval * (px_rows - 1));
        return static_cast<int>(p);
    };

    detail::BrailleGrid grid{cell_rows, cell_cols};

    for (auto const& s : series) {
        // 各点のピクセル行を求める (NaN は -1)
        std::vector<int> ys(s.size(), -1);
        for (std::size_t i = 0; i < s.size(); ++i) {
            if (detail::is_number(s[i])) {
                ys[i] = scaled(s[i]);
            }
        }
        // 点と線分を描画
        for (std::size_t i = 0; i < s.size(); ++i) {
            if (ys[i] < 0) {
                continue;
            }
            grid.set_pixel(static_cast<int>(i), ys[i]);
            if (i + 1 < s.size() && ys[i + 1] >= 0) {
                int const lo = std::min(ys[i], ys[i + 1]);
                int const hi = std::max(ys[i], ys[i + 1]);
                for (int y = lo; y <= hi; ++y) {
                    grid.set_pixel(static_cast<int>(i), y);
                }
                grid.set_pixel(static_cast<int>(i + 1), ys[i + 1]);
            }
        }
    }

    return grid.render();
}

/**
 * @brief 単一系列を Braille 文字で描画する
 * @param series 値の列 (NaN で欠損を表せる)
 * @param cfg    描画設定
 * @return グラフ文字列。空系列や全 NaN なら空文字列
 * @throws std::invalid_argument min が max より大きい場合
 */
inline auto plot_braille(std::vector<double> const& series, Config const& cfg = {})
    -> std::string {
    if (series.empty()) {
        return {};
    }
    bool all_nan = true;
    for (double const v : series) {
        if (detail::is_number(v)) {
            all_nan = false;
            break;
        }
    }
    if (all_nan) {
        return {};
    }
    return plot_braille(std::vector<std::vector<double>>{series}, cfg);
}

} // namespace txtchart

#endif // TXTCHARTPP_TXT_CHART_HPP
