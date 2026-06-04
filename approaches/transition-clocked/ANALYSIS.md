# Analysis: Transition-Clocked Approach

## What This Approach Does

Data is encoded as *transitions* between colours rather than as colours directly.
The LED never stays the same colour twice in a row during transmission — a change
in colour is itself the clock event, and the identity of the new colour (relative
to the old one) carries the 2-bit data value.

Five colours form the data set: Blue, Green, Cyan, Red, Magenta. Yellow and White
are reserved as word markers (and carry the parity bit). Dark means the channel
is closed.

The encoding uses modular arithmetic: given the previous colour's index (0–4 in
the 5-colour ring), the next colour is chosen as `(2-bit value + offset) % 5`,
guaranteeing no colour ever repeats.

## Why It Is Interesting

This was designed under a specific set of constraints:

- No dedicated clock line — the colour change *is* the clock.
- Timing insensitive — the receiver does not need to sample at regular intervals;
  it just watches for any colour change.
- LED is fully on or fully off — no PWM or brightness levels needed.
- Dark = channel closed — the receiver knows instantly when transmission ends.

Embedding the clock in the data transition is a real engineering idea (similar in
spirit to Manchester encoding or FM synthesis in disk storage). The attempt here
is notable because it was worked out from first principles without referencing
those prior arts.

## Where the Complexity Accumulates

**Stateful decode.** The meaning of every incoming colour depends on the previous
colour. A single missed or misread transition shifts the decoding ring, and every
subsequent 2-bit value is wrong until the next Yellow or White mark resets
context. The parity bit can detect this, but recovery is "discard the byte and
try again" — the error is not self-correcting.

**Mirror logic.** The encoder (`nextColourSeq_from_2bit`) and decoder
(`nextColourSeq_to_2bit`) both contain the same large switch block to convert a
colour into a ring offset. This duplication exists because the two directions of
the modular operation need to be handled separately, and there is no cleaner
representation.

**The unsigned subtraction guard.** In the decoder at the point where the
received ring position is subtracted from the expected offset:

```c
if (nibblebase >= offset) {
    halfnibble = (nibblebase - offset) % 4;
} else {
    halfnibble = (5 + nibblebase - offset) % 4;
}
```

This guard exists because `nibblebase` and `offset` are `uint8_t`, and subtraction
going below zero wraps around rather than going negative. The branch manually
implements modular subtraction. It is correct, but it signals that the underlying
representation (unsigned integers used in a signed modular context) is working
against the algorithm.

## The Core Trade-off

Transition clocking saves the dark separator pulse that would otherwise bracket
every colour — roughly halving the number of LED state changes needed per byte.
That is the speed win.

The cost is that the decoder is stateful, the error propagation model is
unfriendly, and the implementation is harder to reason about than an approach
where each colour pulse means the same thing regardless of what came before.

Given that the stated project philosophy is *minimal complexity over speed*, this
approach actually runs counter to its own goals — which is part of what makes it
an interesting historical artefact. It was designed before that philosophy was
fully settled, and its complexity is a concrete illustration of what the
philosophy is pushing against.

## The Ergonomic Advantage That Gets Overlooked

There is one genuine advantage of transition clocking that has nothing to do with
speed: the LED stays continuously lit during transmission. It just changes colour.

A dark-clocked approach turns the LED off between every pulse, which at any
reasonable data rate produces a visible strobe effect. For a status LED on a
device that lives in someone's peripheral vision, that is actively disruptive.
Transition clocking, by contrast, produces a smoothly shifting colour wash that
is much less intrusive as an ambient indicator.

This matters most for the original intended use case — a debug/status LED running
continuously in the background on embedded hardware. In that context, the
continuously-lit behaviour is a real ergonomic benefit, even if it came about as
a side effect of the clocking design rather than a deliberate choice.

## Relation to Other Approaches

- `approaches/dark-clocked/` explores what happens when Dark is used as an
  explicit separator instead. The decoder becomes stateless, the error model
  becomes local, and the implementation shrinks considerably — at the cost of
  roughly 2× the LED transitions per byte.
