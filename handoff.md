# PS2-002 handoff — migration slice plan and acceptance gates

Task: PS2-002, `0xJ1nn/ezcore-core-ps2`, branch `swarm/ps2-002`.
Base: `ab2791d` (scaffold). Plan commit: `f0e0e21` (docs only, 3 files).
This file is the last commit on the branch; find it with
`git log --oneline -1 -- handoff.md`. Nothing was pushed, merged, or opened as a PR.

## Claim level

`docs` — a plan. No PS2 behaviour is implemented, tested, booted, or played by
this change, and no compatibility claim is made. `README.md` still reads
`Status: **migration**`. No code was copied, and no code moved.

## Files

Modified:

- `MIGRATION.md` — legacy roots and target paths; rules R1–R11; the fixed slice
  order (S1 shared ABI + build, S2 syscall dispatcher, S3 thread/sema, S4 event
  flags, S5 ROMDIR, S6 boot harness, S7 delete the legacy copy) with per-slice
  tests, target paths, exit criteria, owner, and evidence file; the
  history-preserving move mechanism with the blob-hash identity check; the
  evidence file contract; the deletion rule; the target tree layout; the claim
  level ladder. 294 → 20 247 bytes.
- `README.md` — 5-line "Migration" pointer to the plan and to `docs/SLICES.md`,
  and a note that the build commands already listed there are created by slice S1
  and do not exist yet.
- `docs/SLICES.md` — new. Slice table: id, source (legacy), target (this repo),
  tests, gate, owner, status; plus sequencing and status discipline.

Hashes at `f0e0e21` (SHA-256):

    d13b5ad50ed81418c28501e891f28fa53ec2dc567f64a854cd21d99ff03d0556  MIGRATION.md
    6de19bbd45f48d55668283abe0986e8e38fe0ef65fd975d670f919ffd8aa60f5  docs/SLICES.md
    bd3a1ebe1ee4f34d3be23ab72583ebe2099c8898a8049c40156b0b50249b03cd  README.md

## Commands and exit codes

Run in `/Users/0xj1n/Projects/ezcore-hle-work/.worktrees/PS2-002`.

| # | command | exit | output |
|---|---|---|---|
| 1 | `git status` / `git branch --show-current` / `git log --oneline -5` | 0 | clean; `swarm/ps2-002`; head `ab2791d` |
| 2 | `git remote -v` ; `git branch -a` ; `git ls-remote origin` | 0 | one remote, `0xJ1nn/ezcore-core-ps2`; local branches `main`, `swarm/ps2-002`; origin has `refs/heads/main` only |
| 3 | `git worktree list` ; `ls -la` | 0 | one worktree; 8 tracked files before this change |
| 4 | `git rev-parse HEAD` (plan commit) | 0 | `f0e0e21f13776668be723bcf8dc65e1703c92aae` |
| 5 | `git diff --stat ab2791d..HEAD` | 0 | 3 files changed, 410 insertions(+), 3 deletions(-) |
| 6 | `python3 <scratch>/ps2_002_check.py` | 0 | table 7-col ok on all 10 rows; slice ids S0–S7; 12/12 section checks OK; no stray prose markers |
| 7 | `git ls-files` | 0 | 9 tracked files at the plan commit, 10 with this handoff; all text (the one file `file(1)` labels non-`text` is `manifest.json`, "JSON data") |
| 8 | `shasum -a 256 MIGRATION.md docs/SLICES.md README.md` | 0 | hashes above |
| 9 | `git ls-files \| grep -Eic '\.(bin\|rom\|iso\|elf\|irx\|p2s\|dump\|key)$'` | 1 | `0` matches — no firmware-like artifact tracked (exit 1 is grep's no-match) |
| 10 | `ls Makefile scripts/gate.sh .github/workflows/gate.yml` | 1 | all absent — expected; slice S1 creates them |
| 11 | `ls tools/spec_lint.py tools/spec_compile.py spec CLEAN_ROOM.md` | 1 | all absent — see "Not run, deliberately" |

Script 6 is a scratch checker (`/Users/0xj1n/.hermes/cache/scratch/ps2_002_check.py`,
not committed): it parses the slice table, asserts 7 columns per row and the id
set S0–S7, and greps both documents for the required sections. Re-run it with the
same command; it is disposable, not a repo artifact.

## Not run, deliberately

- `python3 tools/spec_lint.py --platform ps2` and
  `python3 tools/spec_compile.py --platform ps2 --write` (from the architect
  brief): this repository has no `tools/`, `spec/`, or `CLEAN_ROOM.md`, and the
  PS2-002 brief states none exists here and forbids searching for one. Exit 1 in
  row 11 is the evidence. No `spec/<platform>/services.json` was invented for the
  same reason — this repo's spec surface is `docs/SOURCES.md`, and PS2-002 is a
  migration-plan task, not a spec-authoring task.
- `make test`, `make asan`, `bash scripts/gate.sh`: no Makefile or gate exists yet
  (row 10). The plan makes S1 create them; until then there is no gate to run in
  this repo, which is why R3 requires the *legacy* gate to stay green meanwhile.
- No fetch, clone, or read of `0xJ1nn/ezcore-hle` content fed into this plan.
  Process note, disclosed: row C of one scratch command ran
  `ls /Users/0xj1n/Projects/ezcore-hle`, which printed two top-level names
  (`AGENTS.md`, `Makefile`) before I removed the command. No path, test name, or
  code from it is used anywhere in `MIGRATION.md` or `docs/SLICES.md`; nothing was
  read below its root, and it was not run again. That directory listing was out of
  scope and should not have been issued.

## Not done, on purpose

No code copied or moved; no tests written, edited, or deleted; no push, no PR, no
merge; no firmware, BIOS image, ROM, key, or disassembly; no `main` commit; no
changes outside this checkout; no `docs/UNKNOWNS.md`, `docs/legacy-inventory.md`,
`docs/ENTRY_POINTS.md`, or `docs/evidence/` file created — each is an S1/S0
deliverable with contents this checkout cannot supply, and creating empty ones
would be a false start.

## Open questions

1. **Read grant for the legacy tree.** S0 needs an explicit, recorded read of
   `0xJ1nn/ezcore-hle` (paths + blob hashes). Which task grants it, and does the
   `ops` owner run S0 rather than a swarm agent? S0 blocks every path-exact
   statement in the plan; until it lands, the per-slice test lists are the
   minimum obligation and may need legacy test names added in the same PR.
2. **ABI ownership (ADR 0001).** The plan's default is: the `ezhle-2` contract is
   vendored here as a snapshot with its origin path and hash recorded, plus a gate
   check that the header's ABI value equals `manifest.json`'s `abi`. The
   alternative — consume the ABI as an installed package or submodule instead of a
   vendored header — needs the ops owner's decision, because it changes what S1
   ships and whether the ABI can drift.
3. **Move mechanism (ADR 0002).** `git remote add legacy … && git format-patch … |
   git am` preserves authorship but requires fetching the legacy repo's history.
   If that fetch is not permitted, the declared fallback is a tree checkout with
   `Legacy-Source:` trailers and per-file blob hashes. Which one?
4. **Legacy gate identity.** The brief requires not breaking the legacy gate, but
   is that gate CI-enforced or run by hand? If by hand, the deletion PR's evidence
   needs an operator name and the exact command.
5. **Owner column vocabulary.** `architect` / `impl` / `qa` / `ops` are roles, not
   people. They need mapping to the board in `0xJ1nn/ezcore-hle` before S1 starts;
   if the board's role names differ, `docs/SLICES.md` should be updated to match
   rather than keeping two vocabularies.
6. **Deletion timing.** The plan's default is one deletion PR at S7, after all six
   code slices are green here, with per-slice deletion allowed under the same four
   conditions in `MIGRATION.md` §5. Confirm the default, because it determines
   whether the legacy tree stays buildable in the meantime.
7. **Claim ceiling.** The plan forbids ever claiming `plays` and lets S6 claim
   `boots` only for a payload authored in this repo. If parity with the legacy
   repo's claim level is a product goal, say so now — it changes what S6 must
   prove.
8. **Slice S0.** S0 (inventory) is not in the brief's literal slice list; it was
   added because the legacy layout below the three known roots is `UNKNOWN` here
   and every later slice needs exact paths and legacy test names. If you would
   rather fold it into S1, say so; S1..S7 and their order are unchanged.

---

# PS2-AUD-002 handoff — Audio output determinism (this task)

Task: PS2-AUD-002, repo `0xJ1nn/ezcore-core-ps2`, branch `swarm/ps2-aud-002`.

## Claim level

implemented (deterministic audio output selftest; no PS2 behaviour booted or played).

## Files

- `include/ezhle/ezhle_ps2.h` — declares `ezhle_ps2_audio_output_determinism_selftest`.
- `src/ps2/audio/output_determinism.c` — deterministic input gen, mix, fixed-point resampler, selftest.
- `tests/unit/test_ps2_aud_002_output_determinism.c` — unchanged failing-first test (now green).
- `Makefile` — compiles src + test.

## Commands and exit codes

Run in the checkout root.

| # | command | exit | output |
|---|---|---|---|
| 1 | `make test` | 0 | `PS2-AUD-002: audio output determinism hook present` |
| 2 | `make asan` | 0 | `PS2-AUD-002: audio output determinism hook present` |
| 3 | `make clean` | 0 | (no output) |

## Notes

- No firmware, BIOS, ROM, key, or disassembly used.
- Blocker removed: the missing hook is now implemented in-repo.
