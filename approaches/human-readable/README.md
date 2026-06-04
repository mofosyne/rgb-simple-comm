# Human-Readable Approach

Designed for a human observer — specifically a field technician who can watch
a status LED and verbally read out the colour sequence over a phone, or
transcribe it for later entry.

## Design choices

**Cyan is excluded.** Most people call it "light blue" in natural speech, and
LED cyan can visually pass for blue under real conditions (the blue component
tends to dominate at a distance). Dropping it eliminates that ambiguity.

**Dark as separator.** Darkness is perceptually unambiguous as a gap between
pulses — far clearer than a white or bright separator, which can visually blend
with adjacent colours. A human observer immediately reads a dark period as "space
between colours."

**Green and Red as idle and byte markers.** These are the two most universal
status colours. When the device is idle they hold steady — green for normal,
red for fault. At the end of each byte they serve as the byte marker (and carry
the parity bit), so the transition back to idle is also the byte boundary.

**Data colours: Blue, Magenta, Yellow, White.** Perceptually and verbally
distinct. "Blue, magenta, yellow, white" has no obvious collision in natural
speech. (Magenta may be called "purple" or "pink" by some — acceptable, since
neither collides with any other colour in the set.)

## Colour table

| COLOUR  | R | G | B | Meaning                             |
|---------|---|---|---|-------------------------------------|
| DARK    | 0 | 0 | 0 | Separator between pulses            |
| BLUE    | 0 | 0 | 1 | Data: 00                            |
| GREEN   | 0 | 1 | 0 | Idle (normal) / byte mark parity=0  |
| CYAN    | 0 | 1 | 1 | (unused — too close to blue)        |
| RED     | 1 | 0 | 0 | Idle (fault)  / byte mark parity=1  |
| MAGENTA | 1 | 0 | 1 | Data: 01                            |
| YELLOW  | 1 | 1 | 0 | Data: 10                            |
| WHITE   | 1 | 1 | 1 | Data: 11                            |

## Stream format

```
[steady GREEN or RED] → DARK DATA DARK DATA DARK DATA DARK DATA DARK MARK → [steady GREEN or RED]
```

Each byte = 4 × (DARK + colour) + DARK + GREEN or RED.
GREEN at byte end = parity bit 0. RED = parity bit 1.

The steady idle colour before and after transmission also signals device status —
a human sees green and knows the device is healthy before reading the sequence.

## Relation to other approaches

- Shares the dark-separator structure with `dark-clocked/` but uses a different
  data colour set chosen for human perception rather than machine convenience.
- Slower than `transition-clocked/` (dark separators cost extra transitions)
  but the pulse rate is already intended to be slow enough for human observation.
