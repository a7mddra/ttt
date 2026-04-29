# Personalized Unbeatable Tic-Tac-Toe

This project is a C++ prototype for an unbeatable 3x3 tic-tac-toe game designed to be translated later into an ATmega328P hardware build.

The hardware report describes the final physical target: a 3x3 RGB LED grid, push-button input, row multiplexing through transistors, and a timer-driven refresh loop on an ATmega328P. This repository currently focuses only on the pure C++ command-line game logic.

## Current Prototype

- Board size: 3x3
- AI marker: `r`
- User marker: `b`
- Empty cell: `0`
- Winning line marker: `g`
- Input format: `b1` through `b9`

Note: the hardware report describes the physical color convention as AI Blue and human Red. The current CLI keeps the prototype convention requested for this repo: `r` is AI and `b` is user.

Cell numbers are arranged like this:

```text
1 2 3
4 5 6
7 8 9
```

Example board:

```text
r 0 0
0 b 0
0 0 r
```

Example winning display:

```text
g g g
0 b 0
b 0 r
```

## AI Behavior

The AI is still unbeatable in the normal minimax sense: it never chooses a losing move if a draw or win exists.

The personality layer is added only after the main outcome is safe:

1. Prefer a forced win over a draw.
2. Prefer a draw over a forced loss.
3. Prefer faster wins.
4. Prefer slower losses if losing is unavoidable.
5. Among equal-safe moves, prefer branches with more AI wins.
6. Prefer moves that create threats and forks.
7. Prefer moves that reduce user threats and forks.
8. Use position value and tiny randomness only as final tie-breakers.

This means the AI should draw against perfect play, but punish human mistakes more aggressively than a passive "first safe move" minimax.

## Build

```bash
cmake -S . -B build
cmake --build build
```

The executable is written to the project root:

```bash
./game
```

## Play

Run the game:

```bash
./game
```

Then enter moves:

```text
b5
b9
b2
```

Other commands:

```text
reset
q
```

## Project Files

- `main.cpp` - entry point and high-level project notes.
- `app.h` - CLI renderer, board logic, minimax search, and AI personality scoring.
- `main.ino` - ATmega328P / Arduino AVR sketch using the calibrated keypad and LED matrix.
- `CMakeLists.txt` - CMake build configuration.
- `blueprint.txt` - ATmega328P pin and LED/keypad calibration notes.
- `CALIBRATION.md` - hardcoded keypad and LED matrix mapping used by `main.ino`.
- `Project Report.pdf` - semester report for the intended hardware implementation.

## Hardware Direction

The report's embedded target uses:

- ATmega328P-PU microcontroller.
- 3x3 RGB LED matrix.
- 3 row-driving 2N2222 transistors.
- 3x3 push-button matrix.
- Timer interrupt refresh above visible flicker range.
- Internal pull-ups for input simplification.

The sketch keeps the CLI prototype convention: `r` is the AI red LED, `b` is the user blue LED, and `g` is the green winning line. The keypad columns and LED routes are intentionally hardcoded from `CALIBRATION.md`; do not reorder them to match a clean schematic.

Build a HEX with Arduino CLI, using the Uno target for an ATmega328P at 16 MHz:

```bash
arduino-cli core install arduino:avr
arduino-cli compile --fqbn arduino:avr:uno --output-dir build/avr main.ino
```

If you are using a bare ATmega328P without an Uno bootloader, compile for the matching 16 MHz ATmega328P board package you use for ISP flashing.
