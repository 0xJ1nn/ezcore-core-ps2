# PS2-BIO-009 handoff — Alarms / cache ops / TLB helpers (BLOCKER)

Task: PS2-BIO-009, `0xJ1nn/ezcore-core-ps2`, branch `swarm/ps2-bio-009`.
Base: `f8b6e70` (migration slice plan). Head commit: `bc32fa9a853a7df0000ca98ecb7300f181ae8ff4`.
Nothing pushed, merged, or opened as a PR.

## Claim level

None. No PS2 behaviour implemented, tested, booted, or played. The
interface this task needs (PS2 HLE entry points for alarms, cache ops, TLB
helpers) does not exist in this tree; slices S1/S2 are prerequisites and
are `not-started`. See `docs/PS2-BIO-009-BLOCKER.md`.

## Files

Added:
- `tests/unit/test_ps2_bio_009.c` — RED test (compile fails on missing
  `ezhle/ezhle_ps2.h`).
- `docs/PS2-BIO-009-BLOCKER.md` — blocker document naming the exact
  missing hooks.

## Commands and exit codes

Run in `/Users/0xj1n/Projects/ezcore-hle-work/.worktrees/PS2-BIO-009`.

| # | command | exit | output |
|---|---|---|---|
| 1 | `ls -la` (repo root) | 0 | 9 files, no `src/`, `tests/`, `Makefile`, `scripts/` |
| 2 | `find . -path ./.git -prune -o -type f -print` | 0 | 9 tracked files only |
| 3 | `cc -Iinclude tests/unit/test_ps2_bio_009.c -o /tmp/scratchpad/ps2_bio_009` | 1 | `fatal error: 'ezhle/ezhle_ps2.h' file not found` (RED) |
| 4 | `git add ... && git commit ...` | 0 | `bc32fa9` |
| 5 | `git rev-parse HEAD` | 0 | `bc32fa9a853a7df0000ca98ecb7300f181ae8ff4` |
| 6 | `git status --short` | 0 | clean |

## Not run, deliberately

- `make test`, `make asan`, `bash scripts/gate.sh`: no Makefile or gate
  exists in this tree (missing hook #1 in the blocker doc). Running them
  would only re-confirm the absence of the build system, which is already
  recorded.
- No implementation, no push, no PR, no merge. No firmware/BIOS/ROM/key/
  disassembly read or written.
