# PS2 migration

Status of this document: **plan**. It moves no code by itself. Slice status is in
`docs/SLICES.md`; per-slice evidence is in `docs/evidence/`; the plan's open
questions are in `handoff.md`.

Legacy source of truth: `0xJ1nn/ezcore-hle` — `src/systems/ps2/`,
`include/ezhle/ezhle_ps2.h`, and the PS2 cases under `tests/`.
Target: this repository, `0xJ1nn/ezcore-core-ps2`. It builds and passes its own
gate first; the legacy copy is deleted afterwards.

Do not copy-paste. Move one slice at a time, with its tests, its provenance
record, and green gates on both sides.

**Known limits of this plan.** The three legacy roots above are what is recorded
in this repository. The file structure below `src/systems/ps2/`, the names of the
PS2 cases under the legacy `tests/`, and the legacy build/CI globs that reference
them are `UNKNOWN` here and are not guessed: slice **S0** inventories them under
an explicitly granted read of the legacy tree, and every later slice cites S0
rows (path + blob hash) instead of a remembered path. Until S0 lands, the target
paths and test obligations below are the authoritative contract.

## 0. Terms

- **Slice** — one independently landable move of code plus the tests that cover
  it. One branch, one PR in this repo; at most one deletion PR in the legacy repo.
- **Gate (this repo)** — `make clean && make test && make asan && bash scripts/gate.sh`,
  run at a named commit. `scripts/gate.sh` exits 0 only when every sub-check
  passes and the commit it printed equals `git rev-parse HEAD`.
- **Gate (legacy)** — the gate that protects `0xJ1nn/ezcore-hle`. It must be
  green at every intermediate commit, including immediately after a deletion PR.
- **Evidence file** — `docs/evidence/PS2-002-s<N>.md`, required per slice
  (§4). A slice is not landed until its evidence file is committed.
- **Claim level** — exactly one of `implemented` / `tested` / `boots` / `plays`.
  "migrated", "in tree", or "gate green" are not claim levels and must not be
  written as one.

## 1. Rules

**R1 — Exact paths.** Every slice names its legacy source paths and its target
paths. A legacy path that is not in `docs/legacy-inventory.md` is not moved.

**R2 — Failing-first.** Each slice's tests are committed before its
implementation and the first run is red. The RED run (command, exit code ≠ 0) is
part of the slice evidence. Tests are never edited to pass; a test that is wrong
is fixed in its own commit with the reason recorded.

**R3 — Gate on the exact commit.** Evidence carries the full 40-char SHA of the
commit the gate ran on. "It was green earlier" is not evidence. Any commit that
lands after the gate run invalidates it and the gate is re-run.

**R4 — Delete only after green.** A slice's legacy copy is deleted from
`0xJ1nn/ezcore-hle` **only after** this repo's gate is green on the exact commit
recorded in that slice's evidence file. Both the deletion PR and its evidence
cite that SHA. See §5.

**R5 — Undocumented behaviour.** Anything not backed by a Tier 0/1 source (see
`docs/SOURCES.md`) is written as `UNKNOWN` in `docs/UNKNOWNS.md` with a
deterministic fallback and a test that pins the fallback. The entry point returns
`UNIMPL`. Never guess a value, a number, or a semantic in a spec or in code.

**R6 — No firmware.** No BIOS image, firmware dump, ROM, key, leaked SDK, or
disassembly — in code, tests, fixtures, or evidence. Test fixtures are
synthesised in-process by a committed generator, never extracted from a dump and
never stored as blobs.

**R7 — No copy-paste.** Code moves by the mechanism in §3 and is byte-identical
to the legacy blob at the move commit; a hand-retyped or reformatted file is not
a move. After a move, later edits are ordinary commits in this repo and the
inventory row records the new hash.

**R8 — One slice at a time.** The order in §2 is fixed. A slice does not start
before its predecessor's evidence file is committed. One slice per branch and
per PR.

**R9 — Never both red.** At every point in the sequence, the legacy gate is green
and this repo's gate is green at its head. An intermediate commit that breaks
either is reverted within the same slice.

**R10 — Ownership of deletions.** Deletions in `0xJ1nn/ezcore-hle` are opened by
that repo's owner (`ops`), not by a slice author, and are never merged by the
author of the slice they delete.

**R11 — Claim level.** Each slice evidence file states the claim level after the
slice, and no slice claims more than its tests show. `README.md`'s status line is
updated in the same PR that changes the claim.

## 2. Slice order and per-slice contract

Order: **S0** inventory → **S1** shared ABI + build → **S2** syscall dispatcher →
**S3** thread/sema → **S4** event flags → **S5** ROMDIR → **S6** boot harness →
**S7** delete the legacy copy.

Why this order: S1 is the only slice that can make `make test` meaningful in this
repo, so it comes first even though it carries no PS2 behaviour. S2 is the
contract every later slice is reached through. S3 and S4 are stateful kernel
objects whose save-state layout must exist before the boot harness can serialise
anything. S5 (ROMDIR) is the last parser and needs the error-code contract from
S1/S2. S6 comes last among the code slices because it depends on all of them, and
only S6 may change the claim level above `implemented`. S7 is deletion only.

### S0 — legacy inventory (prerequisite; moves no code)

- Legacy input (read-only): the PS2 surface — `src/systems/ps2/`, the ABI headers
  under `include/ezhle/`, PS2 cases under `tests/`, and the legacy build/CI
  entries that reference them.
- Precondition: an explicit, recorded read grant for the legacy tree. No agent
  reads another checkout without one; S0 runs under its own task with that grant,
  performed by `ops`.
- Target: `docs/legacy-inventory.md`.
- Contents — one row per file: legacy path, blob SHA-1 (`git rev-parse legacy/main:<path>`),
  blob SHA-256, size, rough LOC, assigned slice (S1..S6), legacy test paths that
  cover it, citation IDs already recorded in the legacy tree, licence header
  present (y/n), and any `TODO`/`UNKNOWN` comments carried in the file.
- Also recorded: the legacy commit SHA the inventory was taken from, the count of
  files in the PS2 surface, and the count of rows — these two numbers must match.
- Tests / checks: none executed at S0 (this repo has no test runner yet). The
  check is internal consistency (rows == files, every file assigned to exactly one
  slice, no file left unassigned) plus architect review. `scripts/check_provenance.sh`
  (S1) then operates on this file for every later slice.
- Evidence: `docs/evidence/PS2-002-s0.md` — the commands run, the two counts,
  and any file that could not be assigned a slice.
- Exit criteria: no `UNKNOWN` slice assignment, no Tier-3 material found in the
  surface, legacy commit SHA recorded.

### S1 — shared ABI + build

- Legacy input: `include/ezhle/ezhle_ps2.h`, the ABI contract headers it depends
  on, and the legacy build glue for those paths (exact list from S0).
- Target: `include/ezhle/abi.h`, `include/ezhle/ezhle_ps2.h`,
  `src/ps2/abi/` (exported entry table), `docs/ENTRY_POINTS.md`,
  `Makefile`, `scripts/gate.sh`, `scripts/check_abi.sh`,
  `scripts/check_provenance.sh`, `scripts/check_no_firmware.sh`,
  `scripts/check_evidence.sh`, `.github/workflows/gate.yml`,
  `docs/UNKNOWNS.md`, `docs/adr/0001-abi-ownership.md`,
  `docs/adr/0002-migration-mechanism.md`.
- Tests that must exist and pass at the slice head:
  - `tests/unit/test_abi_version.c` — the ABI version declared by the vendored
    header equals `manifest.json`'s `abi` field and the value recorded in the ADR.
  - `tests/unit/test_entry_table.c` — the exported entry table enumerates only
    documented entry points; every entry's status is `implemented` or `UNIMPL`;
    the `implemented` count equals the count in `docs/ENTRY_POINTS.md`.
  - `tests/unit/test_unimpl_contract.c` — calling an unimplemented entry returns
    the ABI's `UNIMPL` code, records the entry id in the trace, and does not
    abort.
  - plus the migrated legacy ABI/header tests mapped by S0.
- Failing-first: the three tests above land first and `make test` exits non-zero
  (empty `src/ps2/abi/`); the implementation commit makes them pass.
- Gate additions: `check_abi.sh` fails when header ABI ≠ `manifest.json` `abi`;
  `check_no_firmware.sh` fails on any non-text file under `src/`, `include/`,
  `tests/`, or `dist/`, and on the known firmware filename patterns;
  `check_evidence.sh` fails when the current slice's evidence file is missing or
  names a SHA other than `HEAD`.
- Exit criteria: this repo's full gate green; ADR 0001 decided (who owns the
  shared ABI contract after migration, and how each HLE repo consumes it);
  `docs/ENTRY_POINTS.md` published with citation IDs; `docs/UNKNOWNS.md` seeded
  from the legacy TODO/UNKNOWN comments found by S0, each with a fallback.
- Claim level after slice: `implemented` (contract + table; no PS2 behaviour runs).
- Owner: `impl`; ABI/ADR sign-off: `architect`.

### S2 — syscall dispatcher

- Legacy input: the dispatcher, its dispatch/export table, and its tests (exact
  paths from S0).
- Target: `src/ps2/syscall/` (dispatch table + handler registry, internal header).
- Tests: `tests/unit/test_dispatch_lookup.c` (a documented id dispatches to the
  registered handler exactly once, arguments unmodified);
  `tests/unit/test_dispatch_unimpl.c` (an id with no documented behaviour returns
  `UNIMPL`, records the id, invokes no handler, does not abort);
  `tests/unit/test_dispatch_determinism.c` (two runs from identical input produce
  identical traces); plus the migrated legacy dispatcher tests mapped by S0.
- UNKNOWN handling: any dispatcher id that is not in S0's citation-covered set is
  `UNIMPL` + an `UNKNOWNS.md` row (id, what is unknown, fallback = refuse).
- Exit criteria: gate green; `docs/ENTRY_POINTS.md` counts updated if the
  dispatcher moves any entry from `UNIMPL` to `implemented`, with the citation.
- Claim level after slice: `implemented` or `tested`, stated exactly.
- Owner: `impl`.

### S3 — thread / semaphore

- Legacy input: thread and semaphore implementation + tests (paths from S0).
- Target: `src/ps2/kernel/` — thread and semaphore units, plus the kernel
  save-state layout used by `test_kernel_state_roundtrip`.
- Tests: `tests/unit/test_thread_lifecycle.c` (create/start/exit/join/delete;
  ids stable and never reused while live); `tests/unit/test_thread_schedule.c`
  (equal-priority order deterministic across two runs; the documented priority
  rule, else the recorded fallback); `tests/unit/test_sema.c` (wait/post, count,
  wake order, timeout); `tests/unit/test_kernel_state_roundtrip.c`
  (save → load → identical observable state and identical trace); plus migrated
  legacy tests.
- UNKNOWN candidates: scheduling quantum, priority semantics, timeout units — each
  either cited or `UNKNOWN` + deterministic fallback pinned by a test.
- Exit criteria: gate green; ASan clean on the timeout and round-trip paths.
- Claim level after slice: stated exactly, never above `tested`.
- Owner: `impl`.

### S4 — event flags

- Legacy input: event-flag implementation + tests (paths from S0).
- Target: `src/ps2/kernel/` — event flag unit (+ its save-state fields).
- Tests: `tests/unit/test_event_flag_wait.c` (set → waiting thread wakes);
  `tests/unit/test_event_flag_modes.c` (AND/OR and clear-on-return modes per the
  cited definition, else the fallback); `tests/unit/test_event_flag_timeout.c`
  (blocking wait against a deterministic virtual clock);
  `tests/unit/test_event_flag_state_roundtrip.c`; plus migrated legacy tests.
- UNKNOWN candidates: mode-bit semantics, wait ordering, clear-on-wake behaviour.
- Exit criteria: gate green; interactions with S3 primitives covered by at least
  one test that exercises a thread waiting on an event flag.
- Claim level after slice: stated exactly.
- Owner: `impl`.

### S5 — ROMDIR

- Legacy input: ROMDIR reader + tests (paths from S0).
- Target: `src/ps2/romdir/romdir.c` (+ internal header).
- Tests: `tests/unit/test_romdir_parse.c` (in-memory synthetic table: names,
  sizes, entry order, terminator); `tests/unit/test_romdir_malformed.c`
  (truncated, non-terminated, zero-size, and over-long name → typed error, no
  out-of-bounds read, must pass under ASan); `tests/unit/test_romdir_generator.c`
  (the committed generator's bytes round-trip through the parser); plus migrated
  legacy tests.
- Fixtures: generated in-process by a committed generator. No stored blob, no
  image, no dump. The generator's byte layout cites a Tier 0/1 source in
  `docs/SOURCES.md`; the layout is not inferred from any firmware.
- UNKNOWN candidates: record ordering, alignment, name-encoding edge cases.
- Exit criteria: gate green; every parse error is a typed, documented code.
- Claim level after slice: `implemented` or `tested`.
- Owner: `impl`.

### S6 — boot harness

- Legacy input: the boot/harness entry point + tests (paths from S0).
- Target: `src/ps2/boot/harness.c`, `tools/ps2-boot.c`, `docs/BOOT.md`.
- Tests: `tests/integration/test_boot_nofirmware.c` (the harness runs with no
  firmware present and takes the documented `UNIMPL`/no-firmware path — it does
  not crash, hang, or fabricate state); `tests/integration/test_boot_claim_level.c`
  (the harness prints exactly one claim level, and it is not higher than the
  evidence in `docs/evidence/`; `README.md`'s status line matches);
  `tests/integration/test_boot_determinism.c` (two runs, identical log modulo
  timestamps); plus migrated legacy tests.
- Constraint: any non-`UNIMPL` boot path runs a payload authored in this repo.
  No retail title, no dump, no firmware. `boots` may be claimed only for a
  synthetic payload, and the claim names that payload.
- Exit criteria: gate green (including the integration tests); `docs/BOOT.md`
  records what was exercised; `README.md` status updated in the same PR.
- Claim level after slice: the highest level the tests actually show, named
  payload included.
- Owner: `impl`; claim-level sign-off: `architect` + `qa`.

### S7 — delete the legacy copy

- Input: every slice S1..S6 landed here, each with a committed evidence file and a
  gate log at its recorded SHA.
- Action (in `0xJ1nn/ezcore-hle`, opened by `ops`, §5): delete
  `src/systems/ps2/`, `include/ezhle/ezhle_ps2.h`, the PS2 cases under `tests/`,
  and the build/CI globs that reference them, in one PR that lists each slice's
  evidence path + SHA.
- Verification recorded in `docs/evidence/PS2-002-s7.md`, with raw output:
  - `git ls-tree -r <legacy-main-after> -- src/systems/ps2 include/ezhle/ezhle_ps2.h` → no
    matching paths;
  - the legacy gate log after deletion (green, exit 0);
  - this repo's gate at its head after S7 (green, exit 0).
- Then, in this repo: `docs/SLICES.md` statuses → `deleted`; `MIGRATION.md` header
  notes that this repository is the source of truth; `manifest.json` `notes` no
  longer says migration is pending.
- Reversal rule: if a slice is reverted here, its legacy deletion is reverted in
  the same window. Two live copies are never left, and no copy is ever absent
  from both.
- Owner: `ops` (slice author never merges their own deletion).

## 3. Move mechanism (no copy-paste)

Preferred — history-preserving extraction, run on the slice branch inside this
checkout:

    git remote add legacy https://github.com/0xJ1nn/ezcore-hle.git
    git fetch --no-tags legacy main
    git format-patch --binary --stdout legacy/main -- <legacy pathspec from S0> | git am
    git remote remove legacy

then path reconciliation in a separate commit (`git mv` to the target paths of
§2) and the test-mapping commit. Authorship and commit dates survive.

Identity check, recorded in the slice evidence while the legacy remote is still
configured — it proves the file was moved, not retyped:

    git rev-parse legacy/main:<legacy path>      # legacy blob SHA-1
    git rev-parse <move commit>:<target path>    # must be equal

Both hashes go in `docs/legacy-inventory.md` and the evidence file.

Fallback — if history import is rejected, record that decision in
`docs/adr/0002-migration-mechanism.md` and use a tree checkout plus trailers:

    git checkout legacy/main -- <paths>
    git commit -m "move(ps2): <slice>" -m "Legacy-Source: 0xJ1nn/ezcore-hle@<sha>:<path>"

with one `Legacy-Source:` trailer per file and the same blob-hash rows. The
fallback is not a licence to retype: byte-identity is still required at the move
commit.

Both mechanisms require an explicit read grant for the legacy repo; neither is run
by an agent that has not been granted it.

## 4. Evidence file contract

`docs/evidence/PS2-002-s<N>.md`, one per slice, committed in that slice's PR:

1. slice id and title;
2. legacy commit SHA + legacy paths + blob hashes (from S0, or S0's own output for S0);
3. target commit SHA (full) of the commit the gate ran on, and the branch;
4. RED run: command + exit code (non-zero) for the failing-first tests;
5. GREEN run: `make test`, `make asan`, `bash scripts/gate.sh` — command + exit code
   each, plus the commit SHA printed by the gate;
6. target test paths, and the legacy test path each one replaces (or "new, no
   legacy counterpart");
7. claim level after the slice, exactly one of implemented / tested / boots / plays;
8. `UNKNOWNS.md` rows opened or closed by the slice;
9. reviewer sign-off where §2 requires it, with date.

An evidence file with a missing exit code, a short SHA, or a claim level without a
matching test is rejected in review.

## 5. Legacy deletion rule

A slice's code is deleted from `0xJ1nn/ezcore-hle` only when all of these hold:

1. this repo's gate is green on the **exact commit** recorded in that slice's
   evidence file, with the gate log in the evidence;
2. `docs/legacy-inventory.md` rows for the slice are marked moved, with both blob
   hashes recorded;
3. `docs/SLICES.md` shows the slice as `green` here;
4. the deletion PR is opened by `ops` and cites 1–3 by path and SHA.

Default: the legacy copy is deleted once, in S7, after all six code slices are
green here — matching the slice order in §2. Deleting per slice instead is
allowed only under the same four conditions, one PR per slice, and it changes
nothing else in this plan. Necessary in both variants: at no point is the same
slice absent from both repositories, and at no point is either gate red.

## 6. Target layout (this repository)

    include/ezhle/abi.h            ABI contract consumed by every slice
    include/ezhle/ezhle_ps2.h      PS2 public header
    src/ps2/abi/                   exported entry table, UNIMPL fallback
    src/ps2/syscall/               dispatcher
    src/ps2/kernel/                thread, semaphore, event flags, save state
    src/ps2/romdir/                ROMDIR reader
    src/ps2/boot/                  boot harness
    tools/ps2-boot.c               harness front-end (prints the claim level)
    tests/unit/                    per-slice unit tests
    tests/integration/             harness-level tests
    scripts/gate.sh                the gate; exits non-zero on the first failure
    scripts/check_*.sh             abi / provenance / no-firmware / evidence
    docs/                          SOURCES.md, SLICES.md, legacy-inventory.md,
                                   ENTRY_POINTS.md, UNKNOWNS.md, BOOT.md,
                                   adr/, evidence/
    Makefile                       test, asan, gate, dist
    dist/ps2/                      installable package (git-ignored)

## 7. Claim levels and README status

`README.md` says `Status: **migration**` today, and the build commands it lists do
not exist yet — S1 creates them. The status line changes only in the PR that has
the tests to back the new claim:

- `implemented` — code exists, documented entry points dispatch, no test yet;
- `tested` — gate green with unit tests covering the moved behaviour;
- `boots` — the harness runs a named synthetic payload to its documented end;
- `plays` — not claimed by this plan; a title is never used as evidence here.
