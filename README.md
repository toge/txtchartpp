# txtchartpp

asciichartpy 互換の ASCII 折れ線グラフ描画 C++ ヘッダオンリーライブラリ。

```
    9.00  ┤       ╭
    8.00  ┤      ╭╯
    7.00  ┤     ╭╯
    6.00  ┤    ╭╯
    5.00  ┤   ╭╯
    4.00  ┤  ╭╯
    3.00  ┤ ╭╯
    2.00  ┤╭╯
    1.00  ┼╯
```

## 特徴

- **asciichartpy 1.5.25 と同一出力** — `plot()` の戻り値は Python 版とバイト単位で一致
- **点字 (Braille) 対応** — `plot_braille()` で高解像度の折れ線描画
- **ヘッダオンリー** — `#include "txtchartpp/txtchart.hpp"` のみ。本体に依存なし
- **C++26** — `std::format`, 標準ライブラリのみ使用
- **多系列対応** — 系列ごとに異なる色を付与可能 (ANSI)
- **NaN 対応** — 欠損値を NaN で表現するとスキップされる

## ビルド

```bash
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake
cmake --build build
ctest --test-dir build
```

テストは Catch2 (vcpkg の `catch2` パッケージ) を使用する。

## 使い方

```cpp
#include "txtchartpp/txtchart.hpp"
#include <iostream>
#include <vector>

int main() {
    std::vector<double> series = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    std::cout << txtchart::plot(series) << '\n';
}
```

### 多系列

```cpp
std::vector<std::vector<double>> series = {
    {10, 20, 30, 40, 30, 20, 10},
    {40, 30, 20, 10, 20, 30, 40},
};
txtchart::Config cfg;
cfg.height = 3.0;
cfg.colors = {txtchart::red, txtchart::blue};
std::cout << txtchart::plot(series, cfg) << '\n';
```

### 点字 (Braille) 描画

`plot_braille()` は Braille 文字 (U+2800..U+28FF) で描画する。1 文字あたり
2列×4行のドットを持つため、ASCII 版より縦解像度が 4 倍高い。

```cpp
std::vector<double> series = {1, 2, 3, 4, 5, 6, 7, 8, 9};
std::cout << txtchart::plot_braille(series) << '\n';
```

```
    9.00 ┤⠀⠀⠀⢸⠁
    8.11 ┤⠀⠀⠀⡞⠀
    7.22 ┤⠀⠀⢠⠇⠀
    6.33 ┤⠀⠀⣸⠀⠀
    5.44 ┤⠀⢀⡇⠀⠀
    4.56 ┤⠀⢸⠀⠀⠀
    3.67 ┤⠀⡏⠀⠀⠀
    2.78 ┤⢰⠃⠀⠀⠀
    1.89 ┤⡼⠀⠀⠀⠀
    1.00 ┤⡇⠀⠀⠀⠀
```

`Config` は `min`/`max`/`height`/`offset`/`format`/`symbols` を使用する
(`height` はセル行数、デフォルト 10)。

## Config オプション

| フィールド | 型 | デフォルト | 説明 |
|-----------|-----|-----------|------|
| `min` | `std::optional<double>` | 未設定 (自動) | Y 軸の最小値 (クリップ) |
| `max` | `std::optional<double>` | 未設定 (自動) | Y 軸の最大値 (クリップ) |
| `height` | `std::optional<double>` | `interval` (= max - min) | グラフの高さ (行数) |
| `offset` | `int` | 3 | Y 軸ラベルの左マージン (最小 2) |
| `format` | `std::string` | `"{:8.2f} "` | Y 軸ラベルの書式 (`std::format` 書式文字列) |
| `symbols` | `std::array<std::string, 10>` | asciichartpy 互換 | 描画シンボル (ボックス描画文字) |
| `colors` | `std::vector<std::string_view>` | 空 (色なし) | 系列ごとの ANSI 色。系列数より少なければ循環適用 |

色定数: `black`, `red`, `green`, `yellow`, `blue`, `magenta`, `cyan`,
`lightgray`, `default_`, `darkgray`, `lightred`, `lightgreen`,
`lightyellow`, `lightblue`, `lightmagenta`, `lightcyan`, `white`, `reset`。

### asciichartpy との差分

- **`reversed` 非対応** — asciichartpy 1.5.25 にも未実装 (設定しても無視される) のため、本ライブラリにもない
- **`width` 非対応** — asciichartpy に存在しないオプション。幅は系列長から自動決定
- **`format` は `std::format` 書式** — asciichartpy の Python 書式文字列と互換のものが多い (`{:8.2f}`, `{:8.0f}` など)

## エラー

- `min > max` → `std::invalid_argument` を送出
- 空系列 / 全 NaN 系列 → 空文字列を返す

## テスト

`test/test_plot.cpp` は asciichartpy 1.5.25 の実出力と比較するゴールデンテスト。
期待値は `scripts/gen_test.py` が Python 側で生成する (venv に `asciichartpy` が必要)。
`test/test_braille.cpp` は `scripts/gen_test_braille.py` の参照実装から生成する。
