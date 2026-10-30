# PS2-CPU-004 — TLB helpers (planned)

Status: **planned**. No implementation, no tests, no build. Claim level: `docs`
only. No PS2 behaviour is implemented, tested, booted, or played by this change.

Branch: `swarm/ps2-cpu-004`. Repository: `0xJ1nn/ezcore-core-ps2`.

## 1. Purpose

The Emotion Engine (EE, R5900) uses a Translation-Lookaside Buffer (TLB) for
virtual-to-physical address translation (S2). The host core must provide TLB
helpers so that address translation, TLB refill/probe/write instructions, and
save-state round-trip can be exercised deterministically. This document records
the interface the host core must provide, the units of every field, the error
behaviour, and the explicit list of things the sources do not document. It does
not implement anything; undocumented behaviour returns `EZHLE_ERR_UNIMPL`.

## 2. Source basis

Every hardware fact in this document is cited to a row in `docs/SOURCES.md`:

- `S2` — ps2tek, "Documentation on PS2 internals", Emotion Engine / COP0 / TLB
  sections, Tier 1, https://psi-rockin.github.io/ps2tek/
- `S1` — placeholder row, not used by this document.

No other source is used. Where `S2` is silent, the item is listed in
`## 9. UNKNOWN list` and the helper returns `EZHLE_ERR_UNIMPL`.

## 3. Hardware facts (S2)

- TLB entry count: 48 paired entries. Each entry holds one even page and one odd
  page (S2: "48 entries split into even and odd pages").
- Each entry is indexed 0..47 by `COP0.Index` (for `TLBR`/`TLBWI`) and by
  `COP0.Random` (for `TLBWR`) (S2).
- `COP0.Random` decrements after every instruction that is executed; upper bound
  47, lower bound the value in `COP0.Wired` (S2).
- `COP0.Wired` holds the lower bound of `Random` (0..5? field is 0-5 bits, i.e.
  0..47 in practice) (S2).
- Page sizes selectable via `COP0.PageMask` (S2):
  - `000h` = 4 KiB
  - `003h` = 16 KiB
  - `00Fh` = 64 KiB
  - `03Fh` = 256 KiB
  - `0FFh` = 1 MiB
  - `3FFh` = 4 MiB
  - `FFFh` = 16 MiB

### 3.1 TLB entry fields (S2)

Bit positions as documented by S2. "page" means one page of the size selected by
that entry's MASK.

| Field | Bits (in entry) | Width | Meaning | Unit |
|---|---|---|---|---|
| V0 | 1 | 1 | Even page valid; 0 = not mapped | boolean |
| D0 | 2 | 1 | Even page dirty; 0 = writes raise exception | boolean |
| C0 | 3-5 | 3 | Even page cache mode (2=uncached, 3=cached, 7=uncached accelerated) | enum |
| PFN0 | 6-25 | 20 | Even page frame number | page |
| V1 | 33 | 1 | Odd page valid | boolean |
| D1 | 34 | 1 | Odd page dirty | boolean |
| C1 | 35 | 3 | Odd page cache mode | enum |
| PFN1 | 38-57 | 20 | Odd page frame number | page |
| S | 63 | 1 | Scratchpad; virtual mapping targets scratchpad RAM | boolean |
| ASID | 64-71 | 8 | Address Space ID | dimensionless |
| G | 76 | 1 | Global; when set, ASID is ignored | boolean |
| VPN2 | 77-95 | 19 | Virtual page number / 2; even page VPN = VPN2*2, odd = VPN2*2+1 | page-pairs |
| MASK | 109-120 | 12 | Page size (encoded, see 3) | encoded size |

### 3.2 COP0 registers (S2)

| Register | Field | Bits | Meaning | Unit |
|---|---|---|---|---|
| Index ($0) | index | 0-5 | TLB entry accessed by TLBR/TLBWI | entry (0..47) |
| Random ($1) | random | 0-5 | TLB entry accessed by TLBWR | entry (0..47) |
| EntryLo0 ($2) | G | 0 | Global (entry-level) | boolean |
| EntryLo0 ($2) | V | 1 | Page valid | boolean |
| EntryLo0 ($2) | D | 2 | Page dirty | boolean |
| EntryLo0 ($2) | C | 3-5 | Cache mode | enum |
| EntryLo0 ($2) | PFN | 6-25 | Page frame number | page |
| EntryLo0 ($2) | S | 31 | Scratchpad (only EntryLo0) | boolean |
| EntryLo1 ($3) | G,V,D,C,PFN | as above | Odd page fields | as above |
| PageMask ($5) | page size | 13-24 | Encoded page size | encoded size |
| Wired ($6) | lower bound | 0-5 | Lower bound of Random | entry (0..47) |
| EntryHi ($10) | ASID | 0-7 | Address Space ID | dimensionless |
| EntryHi ($10) | VPN2 | 13-31 | Virtual page number / 2 | page-pairs |
| BadVAddr ($8) | address | 0-31 | Virtual address causing TLB Refill / TLB Modified / TLB Invalid / Address Error | bytes |

Global bit of a TLB entry is set only when both EntryLo0.G and EntryLo1.G are set
(S2).

### 3.3 Exception codes (S2)

| Code | Name |
|---|---|
| 00h | Interrupt |
| 01h | TLB Modified |
| 02h | TLB Refill (instruction fetch or load) |
| 03h | TLB Refill (store) |
| 04h | Address Error (instruction fetch or load) |

BadVAddr is written on TLB Refill, TLB Modified, TLB Invalid, and Address Error
(S2).

## 4. Proposed helper interface (planned, not implemented)

Names are proposed; final ABI names are owned by S1 (`docs/ENTRY_POINTS.md` not
yet present). All helpers operate on a caller-provided state object; no global
mutable state, no allocation in hot paths (per agent rules). Addresses are
32-bit byte addresses. Returns are `int` status codes unless stated.

Proposed status codes (names proposed here, not yet defined in ABI):

- `EZHLE_OK` (0)
- `EZHLE_ERR_UNIMPL` — undocumented behaviour (default for all helpers until
  implemented)
- `EZHLE_ERR_BADARG` — null pointer / invalid enum / malformed entry
- `EZHLE_ERR_RANGE` — index outside 0..47, or buffer too small
- `EZHLE_ERR_STATE` — save-state magic/version/size mismatch

### 4.1 Lifecycle and state

| Helper (proposed) | Inputs | Outputs | Units | Error / exception behaviour | Status |
|---|---|---|---|---|---|
| `ps2_tlb_state_size()` | none | `size_t` | bytes | none | planned |
| `ps2_tlb_init(state, wired_reset)` | `state*` (host-owned, `state_size` bytes); `wired_reset` (uint8, entries) | status | entries | `BADARG` if state null; `RANGE` if wired_reset > 47 | planned; reset value of Wired/RANDOM/initial contents: see UNKNOWN |
| `ps2_tlb_reset(state)` | `state*` | status | — | `BADARG` if null | planned; clears all V bits (proposed; see UNKNOWN) |

### 4.2 Translation and lookup

| Helper (proposed) | Inputs | Outputs | Units | Error / exception behaviour | Status |
|---|---|---|---|---|---|
| `ps2_tlb_lookup(state, va, asid, access)` | `state*`; `va` uint32 (bytes); `asid` uint8; `access` enum {fetch, load, store} | struct `{hit: bool, index: int8 (0..47 or -1), pa: uint32 (bytes), cache_mode: enum, dirty: bool, scratchpad: bool, exception: uint8 (S2 code or 0)}` | bytes / entries | `BADARG` if state null or bad enum; on miss returns `hit=false` with exception code 02h/03h (S2); on TLB Modified returns 01h; does not itself raise a guest exception (caller dispatches) | planned |
| `ps2_tlb_translate(state, va, asid, access, *pa)` | convenience wrapper | status + `*pa` | bytes | same as lookup; `BADARG` if `pa` null | planned |

Matching rule (S2): select entries whose MASK matches the page size implied by VA;
compare VPN2 and (ASID or G=1); validity V0/V1 selects even/odd page; page
offset within MASK is preserved.

### 4.3 TLB instructions

| Helper (proposed) | Inputs | Outputs | Units | Error / exception behaviour | Status |
|---|---|---|---|---|---|
| `ps2_tlb_probe(state, va, asid)` (TLBP) | `state*`; `va` uint32; `asid` uint8 | `index` int (0..47) or -1; writes nothing to state (caller updates COP0.Index) | entries | `BADARG` if state null; -1 on no match | planned |
| `ps2_tlb_read_index(state, index, *entry)` (TLBR) | `state*`; `index` uint8; `*entry` | status; fills `ps2_tlb_entry_t` | entries | `RANGE` if index > 47; `BADARG` if pointers null | planned |
| `ps2_tlb_write_index(state, index, entry)` (TLBWI) | `state*`; `index` uint8; `entry` | status | entries | `RANGE` if index > 47; `BADARG` if malformed entry; write below Wired allowed by hardware (see UNKNOWN) | planned |
| `ps2_tlb_write_random(state, entry)` (TLBWR) | `state*`; `entry` | status | entries | uses current `Random`; `BADARG` if malformed; see UNKNOWN for Random handling | planned |

### 4.4 Random / Wired

| Helper (proposed) | Inputs | Outputs | Units | Error / exception behaviour | Status |
|---|---|---|---|---|---|
| `ps2_tlb_random_get(state)` | `state*` | uint8 (0..47) | entries | `BADARG` (returned as 0? proposed: 0) if null | planned |
| `ps2_tlb_random_set(state, value)` | `state*`; value uint8 | status | entries | `RANGE` if value > 47 or value < Wired | planned; direct set is host convenience |
| `ps2_tlb_random_advance(state)` | `state*` | status | per guest instruction | `BADARG` if null | planned; exact ordering vs exceptions: see UNKNOWN |
| `ps2_tlb_wired_get(state)` | `state*` | uint8 | entries | `BADARG` if null | planned |
| `ps2_tlb_wired_set(state, value)` | `state*`; value uint8 | status | entries | `RANGE` if value > 47 | planned |

### 4.5 Flush (host convenience, not a guest instruction)

| Helper (proposed) | Inputs | Outputs | Units | Error / exception behaviour | Status |
|---|---|---|---|---|---|
| `ps2_tlb_flush(state, scope, arg)` | `state*`; `scope` enum {all, asid, va, range}; `arg` (asid uint8 or va uint32 or (start,end)) | uint32 count invalidated | entries | `BADARG` if null/bad scope; `UNIMPL` for scope not documented | planned; host-only |

### 4.6 Save state

| Helper (proposed) | Inputs | Outputs | Units | Error / exception behaviour | Status |
|---|---|---|---|---|---|
| `ps2_tlb_save_state(state, buf, size, *written)` | `state*`; `buf` byte*; `size` size_t; `*written` size_t* | status; bytes written | bytes | `RANGE` if size < required; `BADARG` if null | planned |
| `ps2_tlb_load_state(state, buf, size)` | `state*`; `buf` byte*; `size` size_t | status | bytes | `RANGE` if size != required; `STATE` if magic/version mismatch; `BADARG` if null | planned |

Required saved fields (minimum): all 48 paired entries (V0,D0,C0,PFN0,V1,D1,C1,PFN1,S,ASID,G,VPN2,MASK per entry), `Wired`, current `Random`, and any host bookkeeping. Byte order and magic are S1 ABI decisions (not yet defined).

## 5. Data structure (planned)

```c
/* Proposed; final field names/types owned by S1 ABI. */
typedef struct {
    /* Per entry, 48 of them. */
    struct {
        unsigned v0 : 1, d0 : 1;
        unsigned c0 : 3;
        unsigned pfn0 : 20;
        unsigned v1 : 1, d1 : 1;
        unsigned c1 : 3;
        unsigned pfn1 : 20;
        unsigned s : 1;
        unsigned asid : 8;
        unsigned g : 1;
        unsigned vpn2 : 19;
        unsigned mask : 12;
    } entry[48];
    unsigned wired : 6;   /* 0..47 */
    unsigned random : 6;  /* 0..47 */
} ps2_tlb_state_t;
```

Bitfield packing and host representation are implementation details; the logical
fields and units above are the contract.

## 6. Units summary

| Quantity | Unit | Range / notes |
|---|---|---|
| virtual address (va) | bytes | 32-bit (S2 MIPS IV subset) |
| physical address (pa) | bytes | PFN * page_size |
| ASID | dimensionless | 0..255 (8 bits, S2) |
| VPN2 | page-pairs | 19 bits (S2) |
| PFN0/PFN1 | pages | 20 bits (S2) |
| MASK | encoded page size | S2 table (3) |
| page size | bytes | 4 KiB..16 MiB (S2) |
| entry index | entries | 0..47 (S2) |
| Wired | entries | 0..47 (S2) |
| Random | entries | 0..47, lower bound Wired (S2) |
| cache mode | enum | 2/3/7 documented (S2); others UNKNOWN |
| count invalidated | entries | 0..48 |
| save-state size | bytes | host-defined |
| cycles / frames / samples | n/a | no timing unit is documented by S2 for TLB operations; see UNKNOWN |

## 7. Error behaviour

- Default for every helper until implemented: returns `EZHLE_ERR_UNIMPL` and
  performs no state change (per migration rule R5 and agent rules).
- Invalid pointers / enums: `EZHLE_ERR_BADARG`.
- Out-of-range index or undersized buffer: `EZHLE_ERR_RANGE`.
- Save-state magic/version/size mismatch: `EZHLE_ERR_STATE`.
- Guest-visible TLB misses / TLB Modified: reported via the `exception` field of
  the lookup result (codes 01h/02h/03h, S2); the helper does not raise a host
  exception and does not abort. Dispatching the guest exception is the caller's
  responsibility.
- No helper guesses a value, page size, or semantic not in S2. Unknown items
  return `EZHLE_ERR_UNIMPL` (or are reported in the UNKNOWN list) instead.

## 8. Claim level

`docs` only. No handler exists yet; `implemented` may not be claimed for any
helper in this document until S1 ABI lands and a test pins the behaviour.

## 9. UNKNOWN list

Items not documented by S2 (and not by any registered Tier 0/1 source). Each
returns `EZHLE_ERR_UNIMPL` / is left to the host policy until a source is
registered.

1. Cycle cost / latency of TLB lookup, refill, probe, write (cycles). S2 gives
   no timing. Unit: cycles — UNKNOWN.
2. Whether `Random` decrement occurs on every executed instruction including
   those in delay slots, exception entry, or ERET. S2 says "after every
   instruction that is executed" only. UNKNOWN.
3. Initial values of `Wired`, `Random`, `Index`, `EntryHi`, `EntryLo0/1`,
   `PageMask`, `BadVAddr` at reset. S2 does not state power-on values. UNKNOWN.
4. Initial contents of the 48 TLB entries at reset (all invalid assumed, not
   stated by S2). UNKNOWN.
5. Whether the host may reject `TLBWI` writes with index < `Wired`. Hardware
   does not document enforcement; convention only. Policy UNKNOWN.
6. Cache mode encodings other than 2/3/7. S2 lists only those. UNKNOWN.
7. `PageMask` encodings other than the seven listed. S2 lists only those.
   UNKNOWN.
8. Exact exception vector base addresses for TLB refill under BEV=0 vs BEV=1.
   S2 mentions bootstrap vectors in `BFC00xxx` only. UNKNOWN.
9. Interaction between the scratchpad `S` bit and cache / physical addressing
   beyond "mapping goes to scratchpad instead of main memory" (S2). UNKNOWN.
10. Whether addresses are sign-extended / how 64-bit (kernel) segments are
    handled. EE is a MIPS IV subset (S2) but segment rules beyond kseg0/kseg1 are
    not captured here. UNKNOWN.
11. TLB modified exception side effects beyond writing BadVAddr. UNKNOWN.
12. Flush semantics: no dedicated guest flush instruction is documented by S2;
    `ps2_tlb_flush` is host convenience only. Scope handling beyond clearing V
    bits is UNKNOWN.
13. Save-state byte order, magic, and version. Owned by S1 ABI; not defined yet.
    UNKNOWN.
14. Thread safety / reentrancy of helpers. Host policy; not documented by S2.
    UNKNOWN.
15. Whether `TLBWR` uses `Random` before or after its per-instruction decrement.
    S2 is silent. UNKNOWN.

## 10. Non-goals

- No firmware, BIOS, ROM, key, leaked SDK, or disassembly is used or produced.
- No game-specific hacks.
- No implementation is provided by this document. Helpers remain `UNIMPL` until
  S1 (ABI) lands and a failing-first test pins each behaviour.
