# PS2-AUD-001 — SPU2 audio via host core: BLOCKER

## Status

Blocked. No implementation is claimed. The shared ABI exposed to host cores
(ezhle-2, `include/ezhle/ezhle.h`) does not provide any host audio output
hook, so SPU2 cannot deliver rendered samples to the host core.

## Missing hook (exact)

- `ezhle_bus_t` (in `include/ezhle/ezhle.h`) has no audio output callback:
  no `push_samples`, `audio_out`, `audio_push`, or equivalent, and no
  sample-rate / frame-count field.
- `ezhle_desc_t` has no audio surface (no `audio`, `audio_cb`,
  `host_audio`, or equivalent member) and no alias group for one.
- There is therefore no documented way for an HLE to hand PCM to the host
  core; SPU2 rendering has nowhere to send its output.

## What would unblock

An ABI change: either add an audio output callback to `ezhle_bus_t` or
introduce a separate `ezhle_audio_t` surface attached to `ezhle_desc_t`,
with a documented sample format, sample rate, and push semantics. That is an
ezhle-3 ABI bump (or a new optional surface) and belongs to the ABI owner
(S1 / ADR 0001), not to this task. Until it lands, SPU2 audio returns
`EZHLE_ERR_UNIMPL` (see `src/ps2/spu2/spu2.c` and
`tests/test_ps2_aud_001.c`).

## Evidence

- Test: `tests/test_ps2_aud_001.c` asserts `ps2_spu2_render_frame` returns
  `EZHLE_ERR_UNIMPL` for both a populated and a NULL host audio descriptor.
- Claim level: `implemented` (UNIMPL fallback only; no PS2 behaviour runs).
- No firmware, BIOS, ROM, key, or disassembly was used.