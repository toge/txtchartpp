// freestanding モード検証。
// hosted ビルド (-DTXTCHARTPP_FREESTANDING) では to_chars フォールバック実装を
// 実行して動作を確認する。wasm32-unknown-unknown ビルドではヘッダの自動有効化と
// hosted 専用 <format> への依存が無いこと (コンパイルが通ること) を確認する。
// 注意: GCC 16 のように <string>/<vector> 自体が hosted 専用となる実装では
// hosted の -ffreestanding 検証は不可能なため、hosted では通常リンクで実行する。

#include "txtchartpp/txtchart.hpp"

// wasm32-unknown-unknown ではヘッダ側で自動有効化される。それ以外は明示が必要。
#ifndef TXTCHARTPP_FREESTANDING
#error "TXTCHARTPP_FREESTANDING is not defined (build with -DENABLE_FREESTANDING=ON)"
#endif

#include <cstdio>

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
    CHECK(chart.find('\n') != std::string::npos);      // 複数行
    CHECK(chart.find("┼") != std::string::npos);       // 軸シンボル
    CHECK(chart.find("╭") != std::string::npos || chart.find("╰") != std::string::npos);

    // 2. 複数系列 + 高さ指定
    std::vector<std::vector<double>> const multi = {
        {10, 20, 30, 40, 30, 20, 10},
        {40, 30, 20, 10, 20, 30, 40},
    };
    txtchart::Config cfg;
    cfg.height = 3.0;
    auto const chart2 = txtchart::plot(multi, cfg);
    CHECK(!chart2.empty());

    // 3. FREESTANDING モードではラベルが to_chars 固定書式 ("{:8.2f} " 相当) で生成される
    {
        auto const label = txtchart::detail::format_label("ignored", 12.3456);
        CHECK(label.size() >= 9);                 // 右寄せ 8 桁 + 末尾空白
        CHECK(label.find("12.35") != std::string::npos);
    }

    if (failed == 0) std::printf("freestanding_check: all ok\n");
    return failed == 0 ? 0 : 1;
}
