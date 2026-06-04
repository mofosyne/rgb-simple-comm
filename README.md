* Title: RGB Simple Communication
* Author: Brian Khuu
* Website: briankhuu.com

## Description

A collection of experimental approaches for transmitting data via RGB LED colour
changes. The target scenario is a smartphone camera pointed at an LED, or any
RGB colour sensor.

The guiding philosophy is **minimal complexity over speed** — the goal is
encoding schemes that are easy to implement correctly on bare-metal hardware,
even if that means slower transmission.

## Approaches

### [`approaches/transition-clocked/`](approaches/transition-clocked/)

The original hand-written implementation (2016). Data is encoded as *transitions*
between colours rather than as colours themselves — the colour change is the
clock event, and the new colour (relative to the previous one) carries the 2-bit
value. No dedicated clock line or dark separator needed.

See [`ANALYSIS.md`](approaches/transition-clocked/ANALYSIS.md) for a detailed
look at how it works, where the complexity accumulates, and what trade-offs were
made.

### [`approaches/dark-clocked/`](approaches/dark-clocked/)

An alternative where DARK (all LEDs off) acts as an explicit separator between
each colour pulse. Each colour means the same thing regardless of what came
before, making the decoder stateless. A missed pulse corrupts only that 2-bit
value rather than cascading through the rest of the byte.

The cost is roughly 2× the LED transitions per byte.

## Colour States (common to all approaches)

| COLOUR  | R | G | B |
|---------|---|---|---|
| DARK    | 0 | 0 | 0 |
| BLUE    | 0 | 0 | 1 |
| GREEN   | 0 | 1 | 0 |
| CYAN    | 0 | 1 | 1 |
| RED     | 1 | 0 | 0 |
| MAGENTA | 1 | 0 | 1 |
| YELLOW  | 1 | 1 | 0 |
| WHITE   | 1 | 1 | 1 |

## Building

```
make          # build all approaches
make test     # run all test harnesses
make clean    # remove all build outputs
```

Or build a single approach:

```
make -C approaches/transition-clocked
make -C approaches/dark-clocked
```
