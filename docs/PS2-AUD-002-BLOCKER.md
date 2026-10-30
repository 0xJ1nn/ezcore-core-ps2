# PS2-AUD-002 blocker — Audio output determinism

Task: PS2-AUD-002, repo `0xJ1nn/ezcore-core-ps2`, branch `swarm/ps2-aud-002`.

## Missing hook

The PS2 public ABI header `include/ezhle/ezhle_ps2.h` (slice S1 deliverable)
does not exist in this repository, and with it the audio output determinism
entry point it must export. There is no audio output sink, deterministic sample
clock, or resampler hook to make deterministic, and no build/ABI scaffolding
(S1) that would expose one.

Exact missing hook: `include/ezhle/ezhle_ps2.h` declaring the PS2 audio output
(SPU2) determinism API, gated by the S1 shared ABI + build slice.

## Evidence

- Test: `tests/unit/test_ps2_aud_002_output_determinism.c` (intentionally RED).
- `make test`: fails at compile — `ezhle/ezhle_ps2.h: No such file or directory`.
- No `src/` audio implementation added; no firmware, BIOS, ROM, key, or
  disassembly used or read.

## Claim level

none (blocked). No PS2 behaviour is implemented, tested, booted, or played.
