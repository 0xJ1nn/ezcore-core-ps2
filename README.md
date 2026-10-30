# ezCORE HLE — PlayStation 2

Status: **migration**. The existing PS2 HLE lives in `0xJ1nn/ezcore-hle` and is being moved here. No compatibility claim.

This repository owns the high-level implementation of PlayStation 2 for ezCORE:
its firmware/OS services, board behaviour, tests, gate, and installable
package. It does not contain a BIOS, firmware dump, ROM, key, or disassembly.

## Scope

- clean-room implementation from public documentation only;
- documented entry points and explicit `UNIMPL` for everything else;
- deterministic tests, save-state round-trip, and a gate that runs on every PR;
- an installable package under `dist/ps2/`.

## Build

```bash
make test
make asan
bash scripts/gate.sh
```

## Company

Board and roles: https://github.com/0xJ1nn/ezcore-hle

## Migration

The PS2 HLE code is being moved here slice by slice. Plan and rules:
`MIGRATION.md`. Slice status table: `docs/SLICES.md`. The build commands above are
created by slice S1 and do not exist yet.

## License

BSD-2-Clause. See `LICENSE` and `NOTICE`.
