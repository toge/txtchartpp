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
#include <tuple>
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
    /** @brief 棒グラフのバー描画シンボル (bar() / bar_braille() で使用) */
    std::string bar_symbol = "█";
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
[[nodiscard]] inline auto min_max(std::vector<std::vector<double>> const& series)
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

/** @brief コードポイントを UTF-8 バイト列にエンコードする */
[[nodiscard]] inline auto utf8_encode(char32_t const cp) -> std::string {
    if (cp < 0x80) {
        return std::string{static_cast<char>(cp)};
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
constexpr std::array<int, 8> braille_dot_bits = {
    0x01, 0x02, 0x04, 0x40,
    0x08, 0x10, 0x20, 0x80,
};

/** @brief ANSI 色付き文字列を生成する (color が空ならそのまま返す) — BrailleGrid::render で使用 */
[[nodiscard]] inline auto colorize(std::string const& ch, std::string_view const color) -> std::string;

/** @brief 1セル=2列×4行のピクセルグリッドでドットを立てる */
struct BrailleGrid {
    int cell_rows = 0;
    int cell_cols = 0;
    std::vector<unsigned char> cells;
    std::vector<std::string_view> cell_colors; // セルごとの色 (空なら色なし)

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
        cells[static_cast<std::size_t>(cy) * cell_cols + cx] |= braille_dot_bits[static_cast<std::size_t>(dot)];
    }

    /** @brief ピクセル座標 (px, py) にドットを立てる (色付き) */
    void set_pixel(int const px, int const py, std::string_view const color) {
        if (py < 0 || py >= cell_rows * 4) {
            return;
        }
        int const cx = px / 2;
        if (cx >= cell_cols) {
            return;
        }
        int const cy = py / 4;
        int const dot = (3 - (py % 4)) + (px % 2) * 4;
        std::size_t const idx = static_cast<std::size_t>(cy) * cell_cols + cx;
        cells[idx] |= braille_dot_bits[static_cast<std::size_t>(dot)];
        if (cell_colors.empty()) {
            cell_colors.assign(cells.size(), std::string_view{});
        }
        cell_colors[idx] = color;
    }

    /** @brief セルグリッドを UTF-8 の Braille 文字列に組み立てる (上から下) */
    [[nodiscard]] auto render() const -> std::string {
        std::string out;
        for (int cy = cell_rows - 1; cy >= 0; --cy) {
            for (int cx = 0; cx < cell_cols; ++cx) {
                std::size_t const idx = static_cast<std::size_t>(cy) * cell_cols + cx;
                unsigned char const b = cells[idx];
                std::string const ch = utf8_encode(static_cast<char32_t>(0x2800 + b));
                if (!cell_colors.empty() && !cell_colors[idx].empty()) {
                    out += detail::colorize(ch, cell_colors[idx]);
                } else {
                    out += ch;
                }
            }
            if (cy != 0) {
                out += '\n';
            }
        }
        return out;
    }
};

/** @brief 単系列を複数系列に昇格する (空/全 NaN の場合は std::nullopt) */
[[nodiscard]] inline auto to_multi(std::vector<double> const& s)
    -> std::optional<std::vector<std::vector<double>>> {
    if (s.empty()) {
        return std::nullopt;
    }
    for (double const v : s) {
        if (is_number(v)) {
            return std::vector<std::vector<double>>{s};
        }
    }
    return std::nullopt; // all NaN
}

/** @brief 全系列の最大長 (NaN を問わず) */
[[nodiscard]] inline auto max_categories(std::vector<std::vector<double>> const& series) -> std::size_t {
    std::size_t n = 0;
    for (auto const& s : series) {
        n = std::max(n, s.size());
    }
    return n;
}

/** @brief 全系列の数値でフォーマットしたラベルの最大幅 */
[[nodiscard]] inline auto max_label_width(std::vector<std::vector<double>> const& series,
                                          std::string const& format) -> std::size_t {
    std::size_t label_width = 0;
    for (auto const& s : series) {
        for (double const v : s) {
            if (!is_number(v)) {
                continue;
            }
            std::string const label = std::vformat(format, std::make_format_args(v));
            label_width = std::max(label_width, label.size());
        }
    }
    return label_width;
}

/** @brief 最小/最大/interval を計算する (min>max なら例外) */
inline auto resolve_min_max(double const data_min, double const data_max, Config const& cfg)
    -> std::tuple<double, double, double> {
    double const minimum = cfg.min.value_or(data_min);
    double const maximum = cfg.max.value_or(data_max);
    if (minimum > maximum) {
        throw std::invalid_argument("The min value cannot exceed the max value.");
    }
    return {minimum, maximum, maximum - minimum};
}

/** @brief ANSI 色付き文字列を生成する (color が空ならそのまま返す) */
[[nodiscard]] inline auto colorize(std::string const& ch, std::string_view const color) -> std::string {
    if (color.empty()) {
        return ch;
    }
    return std::string(color) + ch + std::string(reset);
}

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
    if (series.empty()) {
        return {};
    }

    // シンボル定義 (asciichartpy の default_symbols と同一)
    // [0]=cross [1]=right tee [2]=左向き [3]=右向き [4]=水平
    // [5]=下がり [6]=上がり [7]=上 [8]=下 [9]=垂直
    auto const& symbols = cfg.symbols;

    // NaN を無視した全体の最小/最大 (最小 > 最大もしくは全 NaN なら空文字列)
    auto const [data_min, data_max] = detail::min_max(series);
    if (data_min > data_max) {
        return {};
    }

    auto const [minimum, maximum, interval] = detail::resolve_min_max(data_min, data_max, cfg);
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
    if (!series[0].empty()) {
        double const d0 = series[0][0];
        if (detail::is_number(d0)) {
            result[static_cast<std::size_t>(rows - scaled(d0))][static_cast<std::size_t>(offset - 1)] =
                symbols[0];
        }
    }

    // 各系列を描画
    for (std::size_t i = 0; i < series.size(); ++i) {
        std::string_view const color = cfg.colors.empty() ? std::string_view{}
                                                          : cfg.colors[i % cfg.colors.size()];

        auto const& s = series[i];
        for (std::size_t x = 0; x + 1 < s.size(); ++x) {
            double const v0 = s[x];
            double const v1 = s[x + 1];

            if (!detail::is_number(v0) && !detail::is_number(v1)) {
                continue;
            }
            if (!detail::is_number(v0)) {
                result[static_cast<std::size_t>(rows - scaled(v1))][x + static_cast<std::size_t>(offset)] =
                    detail::colorize(symbols[2], color);
                continue;
            }
            if (!detail::is_number(v1)) {
                result[static_cast<std::size_t>(rows - scaled(v0))][x + static_cast<std::size_t>(offset)] =
                    detail::colorize(symbols[3], color);
                continue;
            }

            int const y0 = scaled(v0);
            int const y1 = scaled(v1);

            if (y0 == y1) {
                result[static_cast<std::size_t>(rows - y0)][x + static_cast<std::size_t>(offset)] =
                    detail::colorize(symbols[4], color);
                continue;
            }

            result[static_cast<std::size_t>(rows - y1)][x + static_cast<std::size_t>(offset)] =
                (y0 > y1) ? detail::colorize(symbols[5], color) : detail::colorize(symbols[6], color);
            result[static_cast<std::size_t>(rows - y0)][x + static_cast<std::size_t>(offset)] =
                (y0 > y1) ? detail::colorize(symbols[7], color) : detail::colorize(symbols[8], color);

            int const start = std::min(y0, y1) + 1;
            int const end = std::max(y0, y1);
            for (int y = start; y < end; ++y) {
                result[static_cast<std::size_t>(rows - y)][x + static_cast<std::size_t>(offset)] =
                    detail::colorize(symbols[9], color);
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
    auto const multi = detail::to_multi(series);
    if (!multi) {
        return {};
    }
    return plot(*multi, cfg);
}

/**
 * @brief 複数系列を Braille 文字で描画する
 * @details
 * Braille 文字は 1 文字あたり 2列×4行のドットを持ち、ASCII 版より高解像度。
 * 系列の各点をピクセル座標に写像し、隣接点を直線で結ぶ。
 * @param series 系列のリスト (NaN はスキップされる)
 * @param cfg    描画設定 (min/max/height/offset/format/symbols を使用)
 * @return グラフ文字列。空系列や全 NaN なら空文字列
 * @throws std::invalid_argument min が max より大きい場合
 */
inline auto plot_braille(std::vector<std::vector<double>> const& series, Config const& cfg = {})
    -> std::string {
    if (series.empty()) {
        return {};
    }

    // NaN を無視した全体の最小/最大 (最小 > 最大もしくは全 NaN なら空文字列)
    auto const [data_min, data_max] = detail::min_max(series);
    if (data_min > data_max) {
        return {};
    }

    auto const [minimum, maximum, interval] = detail::resolve_min_max(data_min, data_max, cfg);

    // 高さはセル行数。各セルが 4 ピクセル行に相当する
    int const cell_rows = std::max(static_cast<int>(std::lround(cfg.height.value_or(10.0))), 1);
    int const px_rows = cell_rows * 4;

    // 幅は最長系列の長さ。各データ点が 1 ピクセル列 = 0.5 セル
    std::size_t const max_len = detail::max_categories(series);
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

    for (std::size_t si = 0; si < series.size(); ++si) {
        auto const& s = series[si];
        std::string_view const color = cfg.colors.empty() ? std::string_view{}
                                                          : cfg.colors[si % cfg.colors.size()];
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
            grid.set_pixel(static_cast<int>(i), ys[i], color);
            if (i + 1 < s.size() && ys[i + 1] >= 0) {
                int const lo = std::min(ys[i], ys[i + 1]);
                int const hi = std::max(ys[i], ys[i + 1]);
                for (int y = lo; y <= hi; ++y) {
                    grid.set_pixel(static_cast<int>(i), y, color);
                }
                grid.set_pixel(static_cast<int>(i + 1), ys[i + 1], color);
            }
        }
    }

    // Braille グリッドを行に分割
    std::string const braille_output = grid.render();
    std::vector<std::string> grid_lines;
    {
        std::string line;
        for (char const c : braille_output) {
            if (c == '\n') {
                grid_lines.push_back(line);
                line.clear();
            } else {
                line += c;
            }
        }
        if (!line.empty()) {
            grid_lines.push_back(line);
        }
    }

    // Y 軸ラベルと軸シンボル
    int const offset = cfg.offset;
    auto const& symbols = cfg.symbols;

    // ゼロラインの計算
    int zero_line = -1;
    if (minimum <= 0.0 && maximum >= 0.0) {
        int const py_zero = scaled(0.0);
        zero_line = cell_rows - 1 - py_zero / 4;
    }

    std::string out;
    for (int i = 0; i < cell_rows; ++i) {
        double const label_value = (cell_rows > 1)
            ? maximum - (i * interval / (cell_rows - 1))
            : maximum;
        std::string const label = std::vformat(cfg.format, std::make_format_args(label_value));

        std::string const axis_symbol = (i == zero_line) ? symbols[0] : symbols[1];

        int const col = std::max(offset - static_cast<int>(label.size()), 0);
        std::string left_part = std::string(col, ' ') + label;
        int const pad = (offset - 1) - col - static_cast<int>(label.size());
        if (pad > 0) {
            left_part += std::string(pad, ' ');
        }
        left_part += axis_symbol;

        if (i != 0) {
            out += '\n';
        }
        out += left_part + grid_lines[static_cast<std::size_t>(i)];
    }
    return out;
}

/**
 * @brief 複数系列を水平棒グラフに描画する (ASCII)
 * @details
 * 各値をゼロラインから右方向に延ばした棒で表現する。値が大きいほど棒が長い。
 * 行の先頭に値ラベルを表示し、その後に棒を描画する。
 * 系列が複数の場合、カテゴリごとに系列分の行を縦に積み重ねて並べる。
 * @param series 系列のリスト (系列ごとに値の列)
 * @param cfg    描画設定
 * @return グラフ文字列。空系列や全 NaN なら空文字列
 * @throws std::invalid_argument min が max より大きい場合
 */
inline auto bar(std::vector<std::vector<double>> const& series, Config const& cfg = {})
    -> std::string {
    if (series.empty()) {
        return {};
    }

    // NaN を無視した全体の最小/最大
    auto const [data_min, data_max] = detail::min_max(series);
    if (data_min > data_max) {
        return {};
    }

    auto const [minimum, maximum, interval] = detail::resolve_min_max(data_min, data_max, cfg);

    // カテゴリ数 (全系列の最大長)
    std::size_t const categories = detail::max_categories(series);

    // ラベルの最大幅
    std::size_t label_width = detail::max_label_width(series, cfg.format);
    label_width += static_cast<std::size_t>(cfg.offset);

    // バーの長さ (文字数)。値 0 は長さ 0。範囲外はクランプする (plot() と同様)
    auto const bar_len = [&](double const v) -> int {
        if (interval <= 0.0) {
            return 0;
        }
        auto const cv = std::min(std::max(v, minimum), maximum);
        return static_cast<int>(detail::py_round(cv / interval * cfg.height.value_or(interval)));
    };

    // カテゴリごとに系列分の行を積み重ねる
    std::string out;
    bool first = true;
    for (std::size_t c = 0; c < categories; ++c) {
        for (std::size_t si = 0; si < series.size(); ++si) {
            if (!first) {
                out += '\n';
            }
            first = false;
            auto const& s = series[si];
            double const v = (c < s.size()) ? s[c] : std::numeric_limits<double>::quiet_NaN();
            std::string row;
            if (detail::is_number(v)) {
                std::string const label = std::vformat(cfg.format, std::make_format_args(v));
                row += std::string(label_width - label.size(), ' ') + label;
                std::string_view const color = cfg.colors.empty() ? std::string_view{}
                                                                  : cfg.colors[si % cfg.colors.size()];
                std::string bar_str;
                int const len = bar_len(v);
                for (int i = 0; i < len; ++i) {
                    bar_str += cfg.bar_symbol;
                }
                out += row + detail::colorize(bar_str, color);
            } else {
                out += std::string(label_width, ' ');
            }
        }
    }
    return out;
}

/**
 * @brief 単一系列を水平棒グラフに描画する (ASCII)
 * @param series 値の列 (NaN で欠損を表せる)
 * @param cfg    描画設定
 * @return グラフ文字列。空系列や全 NaN なら空文字列
 * @throws std::invalid_argument min が max より大きい場合
 */
inline auto bar(std::vector<double> const& series, Config const& cfg = {}) -> std::string {
    auto const multi = detail::to_multi(series);
    if (!multi) {
        return {};
    }
    return bar(*multi, cfg);
}

/**
 * @brief 複数系列を水平棒グラフに描画する (Braille)
 * @details
 * 各値を Braille 文字で右方向に延ばした棒で表現する。
 * Braille 文字は 1 文字あたり 2列のドットを持ち、半分セル単位で細かい長さ表現が可能。
 * 行の先頭に値ラベルを表示し、その後に棒を描画する。
 * 系列が複数の場合、カテゴリごとに系列分の行を縦に積み重ねて並べる。
 * @param series 系列のリスト (NaN はスキップされる)
 * @param cfg    描画設定
 * @return グラフ文字列。空系列や全 NaN なら空文字列
 * @throws std::invalid_argument min が max より大きい場合
 */
inline auto bar_braille(std::vector<std::vector<double>> const& series, Config const& cfg = {})
    -> std::string {
    if (series.empty()) {
        return {};
    }

    // NaN を無視した全体の最小/最大
    auto const [data_min, data_max] = detail::min_max(series);
    if (data_min > data_max) {
        return {};
    }

    auto const [minimum, maximum, interval] = detail::resolve_min_max(data_min, data_max, cfg);

    // カテゴリ数 (全系列の最大長)
    std::size_t const categories = detail::max_categories(series);

    // ラベルの最大幅
    std::size_t label_width = detail::max_label_width(series, cfg.format);
    label_width += static_cast<std::size_t>(cfg.offset);

    // バーの長さ (半分セル単位)。Braille の 1 セル = 2 列。範囲外はクランプする
    auto const half_len = [&](double const v) -> int {
        if (interval <= 0.0) {
            return 0;
        }
        auto const cv = std::min(std::max(v, minimum), maximum);
        return static_cast<int>(detail::py_round(cv / interval * cfg.height.value_or(interval) * 2.0));
    };

    // カテゴリごとに系列分の行を積み重ねる
    std::string out;
    bool first = true;
    for (std::size_t c = 0; c < categories; ++c) {
        for (std::size_t si = 0; si < series.size(); ++si) {
            if (!first) {
                out += '\n';
            }
            first = false;
            auto const& s = series[si];
            double const v = (c < s.size()) ? s[c] : std::numeric_limits<double>::quiet_NaN();
            std::string row;
            if (detail::is_number(v)) {
                std::string const label = std::vformat(cfg.format, std::make_format_args(v));
                row += std::string(label_width - label.size(), ' ') + label;
                std::string_view const color = cfg.colors.empty() ? std::string_view{}
                                                                  : cfg.colors[si % cfg.colors.size()];
                std::string bar_str;
                int const len = half_len(v);
                int const full = len / 2;
                int const half = len % 2;
                for (int i = 0; i < full; ++i) {
                    bar_str += cfg.bar_symbol;
                }
                if (half != 0) {
                    bar_str += "▌";  // 左半分のブロック
                }
                out += row + detail::colorize(bar_str, color);
            } else {
                out += std::string(label_width, ' ');
            }
        }
    }
    return out;
}

/**
 * @brief 単一系列を水平棒グラフに描画する (Braille)
 * @param series 値の列 (NaN で欠損を表せる)
 * @param cfg    描画設定
 * @return グラフ文字列。空系列や全 NaN なら空文字列
 * @throws std::invalid_argument min が max より大きい場合
 */
inline auto bar_braille(std::vector<double> const& series, Config const& cfg = {}) -> std::string {
    auto const multi = detail::to_multi(series);
    if (!multi) {
        return {};
    }
    return bar_braille(*multi, cfg);
}

/**
 * @brief 複数系列を垂直棒グラフに描画する (ASCII)
 * @details
 * 各値をゼロラインから上方向に延ばした棒で表現する。
 * 値が大きいほど棒が高く、負の値はゼロラインから下方向に伸びる。
 * 系列が複数の場合、カテゴリごとに系列分の棒を横に並べる。
 * @param series 系列のリスト (系列ごとに値の列)
 * @param cfg    描画設定
 * @return グラフ文字列。空系列や全 NaN なら空文字列
 * @throws std::invalid_argument min が max より大きい場合
 */
inline auto vbar(std::vector<std::vector<double>> const& series, Config const& cfg = {})
    -> std::string {
    if (series.empty()) {
        return {};
    }

    // NaN を無視した全体の最小/最大
    auto const [data_min, data_max] = detail::min_max(series);
    if (data_min > data_max) {
        return {};
    }

    auto const [minimum, maximum, interval] = detail::resolve_min_max(data_min, data_max, cfg);

    // 行数
    double const height = cfg.height.value_or(interval);
    int const rows = static_cast<int>(std::max(height, 1.0));
    // 値 → 行位置 (0 = 最大値, rows-1 = 最小値)。範囲外はクランプする (plot() と同様)
    auto const scaled = [&](double const v) -> int {
        if (interval <= 0.0) {
            return (rows - 1) / 2;
        }
        auto const cv = std::min(std::max(v, minimum), maximum);
        return static_cast<int>(detail::py_round((maximum - cv) / interval * (rows - 1)));
    };
    // ゼロラインの行位置 (区間外の場合は -1)
    int const zero_y = (minimum <= 0.0 && maximum >= 0.0) ? scaled(0.0) : -1;

    // カテゴリ数 (全系列の最大長)
    std::size_t const categories = detail::max_categories(series);

    // 出力を組み立てる (上から下)
    std::string out;
    for (int r = 0; r < rows; ++r) {
        if (r != 0) {
            out += '\n';
        }
        // Y 軸ラベルと軸シンボル
        double const label_value = maximum - (r * interval / (rows > 1 ? rows - 1 : 1));
        std::string const label = std::vformat(cfg.format, std::make_format_args(label_value));
        out += std::string(std::max(static_cast<int>(cfg.offset - label.size()), 0), ' ') + label;
        out += (r == zero_y) ? cfg.symbols[0] : cfg.symbols[1];

        // カテゴリごとの棒
        for (std::size_t c = 0; c < categories; ++c) {
            for (std::size_t si = 0; si < series.size(); ++si) {
                auto const& s = series[si];
                if (c < s.size() && detail::is_number(s[c])) {
                    int const y = scaled(s[c]);
                    int const lo = std::min(y, zero_y < 0 ? y : zero_y);
                    int const hi = std::max(y, zero_y < 0 ? y : zero_y);
                    std::string_view const color = cfg.colors.empty() ? std::string_view{}
                                                                     : cfg.colors[si % cfg.colors.size()];
                    out += detail::colorize((r >= lo && r <= hi) ? cfg.bar_symbol : " ", color);
                } else {
                    out += ' ';
                }
            }
            if (c + 1 < categories) {
                out += ' ';
            }
        }
    }
    return out;
}

/**
 * @brief 単一系列を垂直棒グラフに描画する (ASCII)
 * @param series 値の列 (NaN で欠損を表せる)
 * @param cfg    描画設定
 * @return グラフ文字列。空系列や全 NaN なら空文字列
 * @throws std::invalid_argument min が max より大きい場合
 */
inline auto vbar(std::vector<double> const& series, Config const& cfg = {}) -> std::string {
    auto const multi = detail::to_multi(series);
    if (!multi) {
        return {};
    }
    return vbar(*multi, cfg);
}

/**
 * @brief 複数系列を垂直棒グラフに描画する (Braille)
 * @details
 * 各値を Braille 文字でゼロラインから上方向に延ばした棒で表現する。
 * Braille 文字は 1 文字あたり 4行のドットを持ち、細かい高さ表現が可能。
 * 系列が複数の場合、カテゴリごとに系列分の棒を横に並べる。
 * @param series 系列のリスト (NaN はスキップされる)
 * @param cfg    描画設定
 * @return グラフ文字列。空系列や全 NaN なら空文字列
 * @throws std::invalid_argument min が max より大きい場合
 */
inline auto vbar_braille(std::vector<std::vector<double>> const& series, Config const& cfg = {})
    -> std::string {
    if (series.empty()) {
        return {};
    }

    // NaN を無視した全体の最小/最大
    auto const [data_min, data_max] = detail::min_max(series);
    if (data_min > data_max) {
        return {};
    }

    auto const [minimum, maximum, interval] = detail::resolve_min_max(data_min, data_max, cfg);

    // セル行数 (各セルが 4 ピクセル行)
    int const cell_rows = std::max(static_cast<int>(std::lround(cfg.height.value_or(10.0))), 1);
    int const px_rows = cell_rows * 4;

    // 値 → ピクセル行 (0 = 最大値, px_rows-1 = 最小値)。範囲外はクランプする (plot() と同様)
    auto const scaled = [&](double const v) -> int {
        if (interval <= 0.0) {
            return (px_rows - 1) / 2;
        }
        auto const cv = std::min(std::max(v, minimum), maximum);
        return static_cast<int>(detail::py_round((maximum - cv) / interval * (px_rows - 1)));
    };
    int const zero_py = (minimum <= 0.0 && maximum >= 0.0) ? scaled(0.0) : -1;

    // カテゴリ数 (全系列の最大長)
    std::size_t const categories = detail::max_categories(series);
    // グリッドのピクセル列数 = 各カテゴリ (系列数×2 + 間隔1)
    int const cell_cols = static_cast<int>((categories * (series.size() * 2 + 1) + 1) / 2);

    detail::BrailleGrid grid{cell_rows, cell_cols};

    // 各カテゴリの各系列の棒を描画 (系列 s はカテゴリ c 内のピクセル列
    // c*(2*series.size()+1) + 2*s から 2 ピクセル幅)
    for (std::size_t c = 0; c < categories; ++c) {
        for (std::size_t si = 0; si < series.size(); ++si) {
            auto const& s = series[si];
            if (c >= s.size() || !detail::is_number(s[c])) {
                continue;
            }
            int const py = scaled(s[c]);
            int const lo = std::min(py, zero_py < 0 ? py : zero_py);
            int const hi = std::max(py, zero_py < 0 ? py : zero_py);
            int const x0 = static_cast<int>(c * (series.size() * 2 + 1) + si * 2);
            std::string_view const color = cfg.colors.empty() ? std::string_view{}
                                                             : cfg.colors[si % cfg.colors.size()];
            for (int y = lo; y <= hi; ++y) {
                grid.set_pixel(x0, y, color);
                grid.set_pixel(x0 + 1, y, color);
            }
        }
    }

    // Braille グリッドを行に分割
    std::string const braille_output = grid.render();
    std::vector<std::string> grid_lines;
    {
        std::string line;
        for (char const c : braille_output) {
            if (c == '\n') {
                grid_lines.push_back(line);
                line.clear();
            } else {
                line += c;
            }
        }
        if (!line.empty()) {
            grid_lines.push_back(line);
        }
    }

    // Y 軸ラベルと軸シンボル
    int const offset = cfg.offset;
    auto const& symbols = cfg.symbols;
    int zero_line = -1;
    if (minimum <= 0.0 && maximum >= 0.0) {
        int const py_zero = scaled(0.0);
        zero_line = cell_rows - 1 - py_zero / 4;
    }

    std::string out;
    for (int i = 0; i < cell_rows; ++i) {
        double const label_value = (cell_rows > 1)
            ? maximum - (i * interval / (cell_rows - 1))
            : maximum;
        std::string const label = std::vformat(cfg.format, std::make_format_args(label_value));

        std::string const axis_symbol = (i == zero_line) ? symbols[0] : symbols[1];

        int const col = std::max(offset - static_cast<int>(label.size()), 0);
        std::string left_part = std::string(col, ' ') + label;
        int const pad = (offset - 1) - col - static_cast<int>(label.size());
        if (pad > 0) {
            left_part += std::string(pad, ' ');
        }
        left_part += axis_symbol;

        if (i != 0) {
            out += '\n';
        }
        out += left_part + grid_lines[static_cast<std::size_t>(i)];
    }
    return out;
}

/**
 * @brief 単一系列を垂直棒グラフに描画する (Braille)
 * @param series 値の列 (NaN で欠損を表せる)
 * @param cfg    描画設定
 * @return グラフ文字列。空系列や全 NaN なら空文字列
 * @throws std::invalid_argument min が max より大きい場合
 */
inline auto vbar_braille(std::vector<double> const& series, Config const& cfg = {}) -> std::string {
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
    return vbar_braille(std::vector<std::vector<double>>{series}, cfg);
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
    auto const multi = detail::to_multi(series);
    if (!multi) {
        return {};
    }
    return plot_braille(*multi, cfg);
}

} // namespace txtchart

#endif // TXTCHARTPP_TXT_CHART_HPP
