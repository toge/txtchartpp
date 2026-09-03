/**
 * @file test/smoke_wasi_minimal.cpp
 * @brief TXTCHARTPP_WASI_MINIMAL モードの検証。
 *
 * -fno-exceptions 付きでビルドされる。TXTCHARTPP_THROW を使う全パス
 * (resolve_min_max の min>max) が例外なしでコンパイルできることを確認する。
 */
#include "txtchartpp/txtchart.hpp"

#include <cstdio>
#include <string>

static int failed = 0;

#define CHECK(cond)                                                     \
    do {                                                                \
        if (!(cond)) {                                                  \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            ++failed;                                                   \
        }                                                               \
    } while (0)

int main() {
    // 1. 折れ線グラフ (既定設定) の描画
    std::vector<double> const series = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    auto const chart = txtchart::plot(series);
    CHECK(!chart.empty());
    CHECK(chart.find('\n') != std::string::npos);
    CHECK(chart.find("┼") != std::string::npos);

    // 2. 複数系列 + 高さ指定
    std::vector<std::vector<double>> const multi = {
        {10, 20, 30, 40, 30, 20, 10},
        {40, 30, 20, 10, 20, 30, 40},
    };
    txtchart::Config cfg;
    cfg.height = 3.0;
    auto const chart2 = txtchart::plot(multi, cfg);
    CHECK(!chart2.empty());

    // 3. WASI_MINIMAL モードではラベルが to_chars 固定書式 ("{:8.2f} " 相当)
    {
        auto const label = txtchart::detail::format_label("ignored", 12.3456);
        CHECK(label.size() >= 9);
        CHECK(label.find("12.35") != std::string::npos);
    }

    // 4. 例外なしでも正常系はリンク・実行できる
    {
        txtchart::Config c2;
        c2.min = 0.0;
        c2.max = 10.0;
        auto const chart3 = txtchart::plot(series, c2);
        CHECK(!chart3.empty());
    }

    if (failed == 0) std::printf("smoke_wasi_minimal: all ok\n");
    return failed == 0 ? 0 : 1;
}
