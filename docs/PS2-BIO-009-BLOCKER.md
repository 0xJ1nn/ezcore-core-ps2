# PS2-BIO-009 — blocker: interface does not exist in this repository

Task: PS2-BIO-009, "Alarms / cache ops / TLB helpers".
Repository: `0xJ1nn/ezcore-core-ps2`, branch `swarm/ps2-bio-009`.
Claim level: **none** — no PS2 behaviour implemented, tested, booted, or played.

## Summary

This repository is at the migration-scaffold stage (commit `f8b6e70`,
slice plan only). Slices S1–S7 of `MIGRATION.md` are `not-started`; there is
no build system, no ABI header, no entry table, and no dispatcher. The
interface that PS2-BIO-009 needs (PS2 HLE entry points for alarms, cache
operations, and TLB helpers) therefore does not exist, and implementing it
here would require re-doing slices S1/S2 (and the relevant kernel surface),
which is out of scope for this task.

Per the deliverable's fallback clause, this blocker document names the exact
missing hooks. No `src/` change is made; the smallest correct action is to
stop and record the blocker.

## RED test (written first, not edited to pass)

- Path: `tests/unit/test_ps2_bio_009.c`
- Command: `cc -Iinclude tests/unit/test_ps2_bio_009.c -o /tmp/scratchpad/ps2_bio_009`
- Exit code: `1`
- Failure: `fatal error: 'ezhle/ezhle_ps2.h' file not found`
  (line 17, `#include "ezhle/ezhle_ps2.h"`).

The test is a compile-time demonstration that the PS2 HLE public header —
and therefore every entry point it would expose — is absent. It is not a
weak or edited test; it is the minimal artifact that shows the missing
hook.

## Exact missing hooks

1. Build + gate (slice S1): `Makefile` (targets `test`, `asan`),
   `scripts/gate.sh`, `scripts/check_abi.sh`, `scripts/check_no_firmware.sh`,
   `scripts/check_evidence.sh`, `scripts/check_provenance.sh`. Without these,
   `make test` and `make asan` are not runnable in this tree.
2. ABI contract (slice S1): `include/ezhle/abi.h` (ABI version `ezhle-2`,
   matching `manifest.json`'s `abi` field) and `include/ezhle/ezhle_ps2.h`
   (PS2 public header exposing the entry points).
3. Exported entry table (slice S1): `src/ps2/abi/` — the table enumerating
   documented entry points with status `implemented`/`UNIMPL`, plus the
   `UNIMPL` fallback. `docs/ENTRY_POINTS.md` must list the alarm/cache/TLB
   entries with citation IDs before any can be implemented.
4. Syscall dispatcher (slice S2): `src/ps2/syscall/` — dispatch table +
   handler registry; a documented id must dispatch to its handler exactly
   once with arguments unmodified.
5. Kernel / BIOS service surface (slices S3/S4 and later): `src/ps2/kernel/`
   and any alarm/cache/TLB service unit, with save-state layout and a
   deterministic virtual clock (no wall clock in guest-visible paths).
6. Sources: `docs/SOURCES.md` has only a placeholder row; no Tier 0/1
   citation is registered for alarm, cache, or TLB semantics. Per R5, any
   undocumented behaviour is `UNKNOWN` and returns `EZHLE_ERR_UNIMPL`.

## Prerequisite order (from MIGRATION.md §2)

S0 (inventory) → S1 (ABI + build) → S2 (dispatcher) → S3 (thread/sema) →
S4 (event flags) → S5 (ROMDIR) → S6 (boot harness) → S7 (delete legacy).

PS2-BIO-009 cannot start until at least S1 and S2 are green, and until
`docs/SOURCES.md` registers a Tier 0/1 source for each alarm/cache/TLB
service it would implement. The alarm/cache/TLB entries also need rows in
`docs/ENTRY_POINTS.md` and `docs/UNKNOWNS.md`.

## What was NOT done

- No `src/` change. No firmware/BIOS/ROM/key/disassembly. No `main` commit,
  no push, no PR, no merge. No edit to any test (none existed).
- `make test`, `make asan`, `bash scripts/gate.sh` were not run: no
  Makefile or gate exists in this tree (missing hook #1 above). The only
  verification performed is the compile of the RED test, which exits 1 as
  recorded.
