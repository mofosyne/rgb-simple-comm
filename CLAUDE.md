# Agent Guidelines for rgb-simple-comm

This repository contains experimental RGB LED communication approaches, structured
so different encoding strategies can be compared side by side.

## The Original Code (`approaches/transition-clocked/`)

The code in `approaches/transition-clocked/rgb-simple-comm.c` is the **original
hand-written implementation** by Brian Khuu (2016). It has sentimental value as
a snapshot of the author's first attempt at this problem and serves as an
interesting reference point for the design thinking of the time.

Please treat it with care:

- **Do not refactor, reformat, or "clean up" this code** unless explicitly asked.
  Its structure, style, and even its rough edges are part of what makes it worth
  preserving.
- Its quirks (the unsigned subtraction guard, the duplicated switch blocks) are
  honest artifacts of how the problem was originally approached — not mistakes to
  fix.
- If asked to analyse or critique it, refer to `approaches/transition-clocked/ANALYSIS.md`
  for the documented observations rather than re-deriving them.
- Bug fixes are acceptable only if explicitly requested, and should be minimal and
  clearly noted.

## General Project Philosophy

- **Minimal complexity over speed.** This is not a production protocol library.
  Simpler is better, even at the cost of throughput.
- New approaches go in `approaches/<name>/` as self-contained directories, each
  with their own `makefile` and a brief explanation of the design.
- Do not consolidate approaches or add shared abstractions across them — the whole
  point is that each approach is readable and self-contained on its own terms.
