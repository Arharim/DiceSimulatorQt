# DiceSimulatorQt

Dice roll simulator: N throws of a `1d6 2d8 1d4` input, empirical frequencies
per sum, and the exact theoretical distribution. Native C++20 / Qt 6 Widgets
port of the C#/WPF original
([Arharim/DiceSimulator](https://github.com/Arharim/DiceSimulator)). Runs on
Linux, Windows, and macOS without Wine.

**0.1.0** - feature parity with the original plus input validation fixes.
Fair dice and sums only: no weighted dice, no modifiers (`2d6+3` ignores the
`+3`), no rerolls.

## Tree

| Path | Role |
| --- | --- |
| `src/dice.*` | Core: parse, simulate, theoretical. Plain C++, no Qt |
| `src/mainwindow.*` | Qt Widgets window: inputs, text and chart views |
| `src/chartwidget.*` | Bar chart drawn with QPainter (no QtCharts dependency) |
| `src/main.cpp` | `QApplication` entry |
| `tests/test_dice.cpp` | Unit tests of the core, analytic expectations |
| `assets/` | d20 icon from the original |
| `justfile` | `just build`, `just run`, `just test`, `just fmt`, `just clean` |

## Build

C++20, Qt 6 (`qt6-base` on Arch: Widgets + Concurrent), CMake >= 3.21, Ninja.

```text
just build
just run
```

Or by hand:

```text
cmake -B build -G Ninja
cmake --build build
./build/DiceSimulatorQt
```

## Usage

The window takes dice notation (`1d6 2d8 1d4`, `d6` means `1d6`, `d`/`D` do
not matter) and a throw count, then fills two panes. The left pane lists the
simulation: the number of throws, the empirical mean, and how often each sum
came up. The right pane lists the theory: `M[X]`, `D[X]`, sigma, the sum of
probabilities, and the exact probability of every sum. The toggle button
`🔄` switches both panes between text and bar charts; both charts share one
scale (shares and probabilities), so they compare directly.

The simulation and the theory run in a worker thread. The button reads
"Вычисляю..." while busy and the inputs stay disabled. Results stay until the
next roll.

## Tests

The core is plain C++ and the tests pin it against analytic values: the exact
2d6 table (1/36 .. 6/36 .. 1/36), the uniform 1d20 spread, `M` and `D` of a
mixed set, the sum of probabilities, and a 1M-throw simulation against the
theory. Seeded runs are byte-identical.

```text
just test
# or
ctest --test-dir build
```

## Differences from the WPF original

1. Huge numbers (`1d99999999999999`) are a readable error, not an
   `OverflowException` crash. Same for `0d6` and every other malformed die.
2. Switching views with garbage in the throw-count field does not crash.
3. The roll button cannot stay disabled after a failed computation.
4. The theoretical convolution runs off the UI thread, so `50d1000` does not
   freeze the window.
5. The simulation chart is normalized to shares, so it shares a scale with
   the theory chart (the original compared counts against probabilities).
6. `1D6` parses. Constant summands (`2d6+3`) are skipped, like the original,
   but silently ignoring `0d6` became an error.
7. Input limits: 1000 edges per die, 100 dice per group, 200 dice total,
   10 000 000 throws.

The original's lag came from rebuilding the whole `FlowDocument` on every
update and from the seven-year-old LiveCharts 0.9.7, not from the math: the
simulation itself took microseconds in C#. Here a redraw is a QPainter pass.

## License

DiceSimulatorQt is carried over from the WPF original under the
Mozilla Public License 2.0. The text is in `LICENSE`. The C# sources remain
in their repository and keep their license. Copyright (C) 2026 Arharim,
also known as De1MOS and DM.
