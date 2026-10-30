# PS2 migration slices

Contract and per-slice detail: `MIGRATION.md`. Evidence files: `docs/evidence/`.
Status values: `not-started`, `in-progress`, `red`, `green`, `deleted`.

`gate` is the full repo gate — `make clean && make test && make asan && bash scripts/gate.sh` —
run in this repo at the exact commit named in that slice's evidence file. For S0
and S7 the gate column names the check that actually exists for that slice.

Legacy root paths are `0xJ1nn/ezcore-hle`: `src/systems/ps2/`,
`include/ezhle/ezhle_ps2.h`, PS2 cases under `tests/`. Paths below those roots are
`UNKNOWN` until S0 records them in `docs/legacy-inventory.md`; they are not guessed
in this file.

| id | source (legacy) | target (this repo) | tests (must pass at slice head) | gate | owner | status |
|---|---|---|---|---|---|---|
| S0 | PS2 surface: `src/systems/ps2/`, `include/ezhle/`, PS2 `tests/`, legacy build/CI refs — read-only, exact paths recorded | `docs/legacy-inventory.md` | none executed (no runner yet); internal consistency: inventory rows == files in surface, every file assigned exactly one slice, legacy commit SHA recorded | architect review + row/file count check; consumed by `scripts/check_provenance.sh` from S1 on | architect | not-started |
| S1 | `include/ezhle/ezhle_ps2.h` + ABI contract headers + legacy build glue for them | `include/ezhle/abi.h`, `include/ezhle/ezhle_ps2.h`, `src/ps2/abi/`, `Makefile`, `scripts/gate.sh`, `scripts/check_*.sh`, `.github/workflows/gate.yml`, `docs/ENTRY_POINTS.md`, `docs/UNKNOWNS.md`, `docs/adr/0001-abi-ownership.md`, `docs/adr/0002-migration-mechanism.md` | `tests/unit/test_abi_version.c`, `tests/unit/test_entry_table.c`, `tests/unit/test_unimpl_contract.c`, + legacy header tests mapped by S0 | full gate (creates it); `check_abi.sh`, `check_no_firmware.sh`, `check_evidence.sh` first run here | impl (ABI/ADR sign-off: architect) | not-started |
| S2 | legacy syscall dispatcher + dispatch/export table + its tests | `src/ps2/syscall/` | `tests/unit/test_dispatch_lookup.c`, `tests/unit/test_dispatch_unimpl.c`, `tests/unit/test_dispatch_determinism.c`, + legacy dispatcher tests mapped by S0 | full gate | impl | not-started |
| S3 | legacy thread + semaphore units and their tests | `src/ps2/kernel/` (thread, sema, save state) | `tests/unit/test_thread_lifecycle.c`, `tests/unit/test_thread_schedule.c`, `tests/unit/test_sema.c`, `tests/unit/test_kernel_state_roundtrip.c`, + legacy tests mapped by S0 | full gate, ASan clean on wait/timeout and round-trip paths | impl | not-started |
| S4 | legacy event-flag unit and its tests | `src/ps2/kernel/` (event flags + save-state fields) | `tests/unit/test_event_flag_wait.c`, `tests/unit/test_event_flag_modes.c`, `tests/unit/test_event_flag_timeout.c`, `tests/unit/test_event_flag_state_roundtrip.c`, + legacy tests mapped by S0 | full gate | impl | not-started |
| S5 | legacy ROMDIR reader and its tests | `src/ps2/romdir/` | `tests/unit/test_romdir_parse.c`, `tests/unit/test_romdir_malformed.c`, `tests/unit/test_romdir_generator.c`, + legacy tests mapped by S0 | full gate; fixtures synthesised in-process by the committed generator (no stored blob) | impl | not-started |
| S6 | legacy boot/harness entry point and its tests | `src/ps2/boot/`, `tools/ps2-boot.c`, `docs/BOOT.md` | `tests/integration/test_boot_nofirmware.c`, `tests/integration/test_boot_claim_level.c`, `tests/integration/test_boot_determinism.c`, + legacy tests mapped by S0 | full gate incl. integration tests; claim level may only be raised here, and only for a named synthetic payload | impl (claim sign-off: architect + qa) | not-started |
| S7 | legacy PS2 tree: `src/systems/ps2/`, `include/ezhle/ezhle_ps2.h`, PS2 `tests/`, legacy build/CI refs — deleted, not moved | deletion PR in `0xJ1nn/ezcore-hle` (no files land here); status flips in `docs/SLICES.md`, `MIGRATION.md`, `manifest.json` notes | residue check `git ls-tree -r <legacy-main-after> -- src/systems/ps2 include/ezhle/ezhle_ps2.h` returns no paths; legacy gate green after deletion; this repo's gate green at its head | deletion allowed only under `MIGRATION.md` §5 (green gate on the exact recorded commit per slice, inventory rows marked moved, statuses `green`) | ops (never the slice author) | not-started |

## Sequencing

S0 must land before S1..S7 (it is what makes their paths and legacy test mappings
exact). After S0 the order is fixed: S1 → S2 → S3 → S4 → S5 → S6 → S7, one branch
and one PR each, and a slice may not start before its predecessor's evidence file
is committed.

## Status discipline

`green` means this repo's gate ran green on the exact commit recorded in
`docs/evidence/PS2-002-s<N>.md`. `deleted` means the legacy copy is gone and the
residue check output is in `docs/evidence/PS2-002-s7.md`. A slice whose legacy copy
is still present here and still present there is `green`, never `deleted`.
