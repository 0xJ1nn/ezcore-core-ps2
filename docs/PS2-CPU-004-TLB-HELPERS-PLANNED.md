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
- `S1` — placeholder row in `docs/SOURCES.md`, not used by this document.

No other source is used. `S1` as used above is a row id in `docs/SOURCES.md`; it
is not the migration slice S1 (shared ABI + build), which this document calls
"slice S1". Where `S2` is silent, the item is listed in `## 9. UNKNOWN list` and
the helper returns `EZHLE_ERR_UNIMPL`.

## 3. Hardware facts (S2)

- TLB entry count: 48 paired entries. Each entry holds one even page and one odd
  page (S2: "The TLB contains 48 entries split into even and odd pages").
- Each entry is indexed 0..47 by `COP0.Index` (for `TLBR`/`TLBWI`) and by
  `COP0.Random` (for `TLBWR`) (S2).
- `COP0.Random` decrements after every instruction that is executed; upper bound
  47, lower bound the value in `COP0.Wired` (S2).
- `COP0.Wired` holds the lower bound of `Random`; the field is bits 0-5 (6 bits)
  and S2 gives `Random` an upper bound of 47 (S2).
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

S2 states that the units of PFN0/PFN1 and VPN2 depend on MASK, and gives a
worked example: with MASK selecting 4 KiB, PFN0=10h, PFN1=11h and VPN2=8h map
virtual 10000h to physical 10000h and virtual 11000h to physical 11000h, so an
access to 10500h touches physical 10500h (S2). The rule this interface uses is
`pa = PFN * page_size + (va & (page_size - 1))`, with `page_size` decoded from
the entry's MASK (S2); every address in this document is in bytes.

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
| Wired ($6) | lower bound of Random | 0-5 (6-bit field) | Lower bound of Random; S2 gives Random an upper bound of 47 | entry (0..47) |
| EntryHi ($10) | ASID | 0-7 | Address Space ID | dimensionless |
| EntryHi ($10) | VPN2 | 13-31 | Virtual page number / 2 | page-pairs |
| BadVAddr ($8) | address | 0-31 | Virtual address that caused an exception | bytes |

Global bit of a TLB entry is set only when both EntryLo0.G and EntryLo1.G are set
(S2). EntryLo0 and EntryLo1 supply the even and odd page halves of the TLB entry
respectively (S2).

### 3.3 Exception codes (S2)

The exception code is `COP0.Cause` bits 2-6 (S2). Codes that bear on TLB
translation:

| Code | Name | TLB relevance |
|---|---|---|
| 00h | Interrupt | none |
| 01h | TLB Modified | write to a mapped page whose D bit is 0 (S2) |
| 02h | TLB Refill (instruction fetch or load) | fetch/load that misses or is invalid (S2) |
| 03h | TLB Refill (store) | store that misses or is invalid (S2) |
| 04h | Address Error (instruction fetch or load) | e.g. privileged segment without privilege (S2) |
| 05h | Address Error (store) | same, for a store (S2) |

S2 also lists 06h Bus Error (instruction), 07h Bus Error (data), 08h Syscall,
09h Breakpoint, 0Ah Reserved Instruction, 0Bh Coprocessor Unusable, 0Ch
Overflow, 0Dh Trap. They are outside this helper surface and are not produced by
it.

`BadVAddr` (bits 0-31, "Virtual address that caused an exception") is written
when a TLB Refill, TLB Modified, TLB Invalid, or Address Error occurs (S2). S2
names "TLB Invalid" in that sentence but assigns it no distinct `ExcCode`;
whether an invalid page reports 02h/03h or something else is UNKNOWN (§9.6).

### 3.4 Exception vectors (S2)

| Exception | Normal vector | Bootstrap vector | Level |
|---|---|---|---|
| Reset/NMI | BFC00000h | BFC00000h | 2 |
| TLB Refill | 80000000h | BFC00200h | 1 |
| Performance counter | 80000080h | BFC00280h | 2 |
| Debug | 80000100h | BFC00300h | 2 |
| All others | 80000180h | BFC00380h | 1 |
| Interrupt | 80000200h | BFC00400h | 1 |

`Status.BEV` (bit 22) selects the bootstrap vectors for level 1 exceptions and
`Status.DEV` (bit 23) for level 2 (S2). Level 1 entry sets `Cause.ExcCode`, sets
`EPC` (PC-4 with `Cause.BD` set when the fault is in a branch delay slot), sets
`Status.EXL`, then jumps to the vector; level 2 entry does the same with
`ErrorEPC`, `Cause.BD2` and `Status.ERL` (S2). The helpers below return the code
and leave vector selection to the caller, because the vector depends on
`Status`, which this module does not own.

### 3.5 Address segments (S2)

| Segment | Range | Mapping |
|---|---|---|
| kuseg | 00000000h-7FFFFFFFh | user, TLB-mapped |
| kseg0 | 80000000h-9FFFFFFFh | kernel, directly mapped, cached |
| kseg1 | A0000000h-BFFFFFFFh | kernel, directly mapped, uncached |
| ksseg | C0000000h-DFFFFFFFh | supervisor, TLB-mapped |
| kseg3 | E0000000h-FFFFFFFFh | kernel, TLB-mapped |

For kseg0 and kseg1 the physical address is `address & 1FFFFFFFh` and no TLB
entry is consulted (S2); those two segments must bypass the helpers in §4.
Accessing a privileged region without the required privilege raises an Address
Error (S2), which the caller reports using §3.3 rather than through the helpers.

## 4. Proposed helper interface (planned, not implemented)

Names are proposed; final ABI names are owned by slice S1 (`docs/ENTRY_POINTS.md`
is not yet present). All helpers operate on a caller-provided state object; no
global mutable state, no allocation in hot paths (per agent rules). Addresses are
32-bit byte addresses. Returns are `int` status codes unless stated.

S2 documents the COP0 TLB instruction *encoding* only — opcode `COP0`
(010000h) with the TLB `rs` field and funct TLBR=1, TLBWI=2, TLBWR=6, TLBP=8 —
plus the `Index`/`Random` registers those instructions use. It does not document
what the instructions do to the registers, so every helper below that goes
beyond that encoding carries an UNKNOWN reference in §9.

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
| `ps2_tlb_init(state, wired_reset)` | `state*` (host-owned, `state_size` bytes); `wired_reset` (uint8, entries) | status | entries | `BADARG` if state null; `RANGE` if wired_reset > 47 | planned; reset value of Wired/Random/initial contents: see §9.3, §9.4 |
| `ps2_tlb_reset(state)` | `state*` | status | — | `BADARG` if null | planned; clears all V bits (proposed; see §9.4) |

### 4.2 Translation and lookup

| Helper (proposed) | Inputs | Outputs | Units | Error / exception behaviour | Status |
|---|---|---|---|---|---|
| `ps2_tlb_lookup(state, va, asid, access)` | `state*`; `va` uint32 (bytes); `asid` uint8; `access` enum {fetch, load, store} | struct `{hit: bool, index: int8 (0..47 or -1), pa: uint32 (bytes), cache_mode: enum, dirty: bool, scratchpad: bool, exception: uint8 (S2 code or 0)}` | bytes / entries | `BADARG` if state null or bad enum; on miss returns `hit=false` with exception code 02h/03h (S2); on TLB Modified returns 01h; does not itself raise a guest exception (caller dispatches) | planned |
| `ps2_tlb_translate(state, va, asid, access, *pa)` | convenience wrapper | status + `*pa` | bytes | same as lookup; `BADARG` if `pa` null | planned |

Matching rule (S2): compare VPN2 and (ASID, or accept the entry when G=1);
V0/V1 select the even/odd page; the offset within MASK is preserved, and the
physical address follows §3.1's `pa` rule. Which MASK values are acceptable, and
how overlapping entries are ordered, is UNKNOWN (§9.11).

### 4.3 TLB instructions

| Helper (proposed) | Inputs | Outputs | Units | Error / exception behaviour | Status |
|---|---|---|---|---|---|
| `ps2_tlb_probe(state, va, asid)` (TLBP, funct 8) | `state*`; `va` uint32; `asid` uint8 | `index` int (0..47) or -1; writes nothing to state (caller updates COP0.Index) | entries | `BADARG` if state null; -1 on no match | planned; on-miss register effect UNKNOWN (§9.7) |
| `ps2_tlb_read_index(state, index, *entry)` (TLBR, funct 1) | `state*`; `index` uint8; `*entry` | status; fills `ps2_tlb_entry_t` | entries | `RANGE` if index > 47; `BADARG` if pointers null | planned; register effects UNKNOWN (§9.8) |
| `ps2_tlb_write_index(state, index, entry)` (TLBWI, funct 2) | `state*`; `index` uint8; `entry` | status | entries | `RANGE` if index > 47; `BADARG` if malformed entry; write below Wired allowed by hardware (see §9.5) | planned |
| `ps2_tlb_write_random(state, entry)` (TLBWR, funct 6) | `state*`; `entry` | status | entries | uses current `Random`; `BADARG` if malformed; see §9.18 for Random handling | planned |

S2 gives only the encoding (TLBR=1, TLBWI=2, TLBWR=6, TLBP=8). What each
instruction does to `Index`, `EntryHi`, `EntryLo0/1` and `PageMask`, and which
exceptions they can raise, is not stated by S2: UNKNOWN (§9.7, §9.8).

### 4.4 Random / Wired

| Helper (proposed) | Inputs | Outputs | Units | Error / exception behaviour | Status |
|---|---|---|---|---|---|
| `ps2_tlb_random_get(state, *value)` | `state*`; `*value` uint8* | status; `*value` 0..47 | entries | `BADARG` if either pointer is null | planned |
| `ps2_tlb_random_set(state, value)` | `state*`; value uint8 | status | entries | `RANGE` if value > 47 or value < Wired | planned; direct set is host convenience |
| `ps2_tlb_random_advance(state)` | `state*` | status | per guest instruction | `BADARG` if null | planned; exact ordering vs exceptions: see §9.2 |
| `ps2_tlb_wired_get(state, *value)` | `state*`; `*value` uint8* | status; `*value` 0..47 | entries | `BADARG` if either pointer is null | planned |
| `ps2_tlb_wired_set(state, value)` | `state*`; value uint8 | status | entries | `RANGE` if value > 47 | planned |

### 4.5 Flush (host convenience, not a guest instruction)

| Helper (proposed) | Inputs | Outputs | Units | Error / exception behaviour | Status |
|---|---|---|---|---|---|
| `ps2_tlb_flush(state, scope, arg)` | `state*`; `scope` enum {all, asid, va, range}; `arg` (asid uint8 or va uint32 or (start,end)) | uint32 count invalidated | entries | `BADARG` if null/bad scope; `UNIMPL` for scope not documented | planned; host-only, see §9.15 |

### 4.6 Save state

| Helper (proposed) | Inputs | Outputs | Units | Error / exception behaviour | Status |
|---|---|---|---|---|---|
| `ps2_tlb_save_state(state, buf, size, *written)` | `state*`; `buf` byte*; `size` size_t; `*written` size_t* | status; bytes written | bytes | `RANGE` if size < required; `BADARG` if null | planned |
| `ps2_tlb_load_state(state, buf, size)` | `state*`; `buf` byte*; `size` size_t | status | bytes | `RANGE` if size != required; `STATE` if magic/version mismatch; `BADARG` if null | planned |

Required saved fields (minimum): all 48 paired entries (V0,D0,C0,PFN0,V1,D1,C1,
PFN1,S,ASID,G,VPN2,MASK per entry), `Wired`, current `Random`, and any host
bookkeeping. Byte order and magic are slice S1 ABI decisions (not yet defined);
see §9.16.

## 5. Data structure (planned)

```c
/* Proposed; final field names/types owned by slice S1 ABI. */
typedef struct {
    unsigned v0 : 1;      /* even page valid          (S2) */
    unsigned d0 : 1;      /* even page dirty          (S2) */
    unsigned c0 : 3;      /* even page cache mode     (S2) */
    unsigned pfn0 : 20;   /* even page frame number   (S2) */
    unsigned v1 : 1;      /* odd page valid           (S2) */
    unsigned d1 : 1;      /* odd page dirty           (S2) */
    unsigned c1 : 3;      /* odd page cache mode      (S2) */
    unsigned pfn1 : 20;   /* odd page frame number    (S2) */
    unsigned s : 1;       /* scratchpad mapping       (S2) */
    unsigned asid : 8;    /* address space id         (S2) */
    unsigned g : 1;       /* global, ASID ignored     (S2) */
    unsigned vpn2 : 19;   /* virtual page number / 2  (S2) */
    unsigned mask : 12;   /* encoded page size        (S2) */
} ps2_tlb_entry_t;

typedef struct {
    ps2_tlb_entry_t entry[48];  /* S2: 48 entries, even/odd page pairs */
    unsigned wired : 6;         /* 0..47 (S2) */
    unsigned random : 6;        /* 0..47 (S2) */
} ps2_tlb_state_t;
```

`ps2_tlb_entry_t` is the type the helpers in §4.3 take and return. Bitfield
packing and host byte order are implementation details; the logical fields,
widths and units above are the contract, and the bit numbers in §3.1 are the
guest-visible layout that `TLBR`/`TLBWI` expose, not the host layout.

## 6. Units summary

| Quantity | Unit | Range / notes |
|---|---|---|
| virtual address (va) | bytes | 32-bit (S2 segment table, §3.5) |
| physical address (pa) | bytes | PFN * page_size + offset (S2 worked example, §3.1) |
| ASID | dimensionless | 0..255 (8 bits, S2) |
| VPN2 | page-pairs | 19 bits (S2) |
| PFN0/PFN1 | pages | 20 bits (S2) |
| MASK | encoded page size | S2 table (3) |
| page size | bytes | 4 KiB..16 MiB (S2) |
| entry index | entries | 0..47 (S2) |
| Wired | entries | 0..47 (S2) |
| Random | entries | 0..47, lower bound Wired (S2) |
| cache mode | enum | 2/3/7 documented (S2); others UNKNOWN (§9.9) |
| exception code | `Cause` bits 2-6 | S2 codes, §3.3 |
| count invalidated | entries | 0..48 |
| save-state size | bytes | host-defined |
| cycles / frames / samples | n/a | no timing unit is documented by S2 for TLB operations; see §9.1 |

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
helper in this document until slice S1 ABI lands and a test pins the behaviour.

## 9. UNKNOWN list

Items not documented by S2 (and not by any registered Tier 0/1 source). Each
returns `EZHLE_ERR_UNIMPL` / is left to the host policy until a source is
registered.

1. Cycle cost / latency of TLB lookup, refill, probe, write. S2 gives no timing
   for any of them. Unit: cycles — UNKNOWN.
2. Whether the `Random` decrement occurs on every executed instruction including
   those in delay slots, on exception entry, or across ERET. S2 says only
   "decrements after every instruction that is executed". UNKNOWN.
3. Initial values of `Wired`, `Random`, `Index`, `EntryHi`, `EntryLo0/1`,
   `PageMask`, `BadVAddr` at reset. S2 states no power-on values. UNKNOWN.
4. Initial contents of the 48 TLB entries at reset (all invalid assumed, not
   stated by S2). UNKNOWN.
5. Whether the host may reject `TLBWI` writes with index < `Wired`. S2 documents
   no enforcement; convention only. Policy UNKNOWN.
6. Which `ExcCode` an invalid page produces. S2 names "TLB Invalid" as a
   BadVAddr-writing exception but lists no code for it; 02h/03h is an assumption,
   not a citation. UNKNOWN.
7. What `TLBP` writes to `COP0.Index` on a miss. S2 gives the encoding only.
   UNKNOWN.
8. What `TLBR`/`TLBWI`/`TLBWR` do to `PageMask`, `EntryHi`, `EntryLo0/1`, and
   which exceptions they can raise. S2 gives the encoding only. UNKNOWN.
9. Cache mode encodings other than 2/3/7. S2 lists only those. UNKNOWN.
10. `PageMask` encodings other than the seven listed. S2 lists only those.
    UNKNOWN.
11. Whether matching compares the entry's own MASK against the VA, and how
    overlapping entries are ordered. S2 gives units and a worked example, not a
    match algorithm. UNKNOWN.
12. Interaction between the scratchpad `S` bit and cache / physical addressing
    beyond "the virtual mapping goes to scratchpad instead of main memory" (S2).
    UNKNOWN.
13. Sign extension of 32-bit addresses and handling of the 64-bit (kernel) high
    half. S2 documents the five segments in §3.5 and nothing above bit 31.
    UNKNOWN.
14. TLB Modified side effects beyond writing `BadVAddr`. UNKNOWN.
15. Flush semantics: S2 documents no dedicated guest flush instruction;
    `ps2_tlb_flush` is host convenience only. Scope handling beyond clearing V
    bits is UNKNOWN.
16. Save-state byte order, magic, and version. Owned by slice S1 ABI; not
    defined yet. UNKNOWN.
17. Thread safety / reentrancy of helpers. Host policy; not documented by S2.
    UNKNOWN.
18. Whether `TLBWR` uses `Random` before or after its per-instruction decrement.
    S2 is silent. UNKNOWN.

## 10. Verification record

Verified on 2026-10-31 against S2 (ps2tek, https://psi-rockin.github.io/ps2tek/,
the "EE COP0 Memory Management", "EE COP0 Exception Handling" and "TLB/Exception"
instruction-encoding sections), cited through row `S2` of `docs/SOURCES.md`.

Confirmed against S2, unchanged: entry count and even/odd pairing; every field
and bit position in §3.1; the PFN/VPN2 unit rule and worked example; the COP0
register field tables in §3.2; the G-bit rule; the seven `PageMask` encodings;
the `Random`/`Wired` sentence; the 48-entry index range.

Corrected in this revision, because the earlier text claimed silence where S2
speaks:

- §3.4 is new. S2 documents the full exception vector table, including TLB
  Refill 80000000h (normal) and BFC00200h (bootstrap), with `Status.BEV`/`DEV`
  selecting bootstrap vectors. The previous UNKNOWN item asserting that these
  addresses were undocumented has been removed.
- §3.5 is new. S2 documents the five address segments and the kseg0/kseg1
  `& 1FFFFFFFh` bypass. UNKNOWN #13 is now limited to sign extension and the
  64-bit high half, which S2 genuinely does not state.
- §3.3 now lists 05h Address Error (store), which S2 documents alongside 04h,
  and names the field (`Cause` bits 2-6). The earlier table stopped at 04h.
- §3.2 `Wired` row no longer carries an unresolved "0..5?" question; S2 gives
  the field as bits 0-5 and `Random` an upper bound of 47.
- §5 now defines `ps2_tlb_entry_t`, which §4.3 already referenced but the
  earlier struct did not declare.
- §4.4 getters take an out-parameter so a null state is reported as `BADARG`
  instead of being indistinguishable from a value.
- §9 gained UNKNOWNs 6, 7, 8 and 11 for points S2 leaves open (TLB Invalid
  code, TLBP on miss, TLBR/TLBWI/TLBWR register effects, match ordering).

Not verified, because no source in `docs/SOURCES.md` covers it: TLB timing, reset
values, and the hardware behaviour of the four TLB instructions beyond their
encoding. Those stay UNKNOWN rather than being inferred from another
architecture.

## 11. Non-goals

- No firmware, BIOS, ROM, key, leaked SDK, or disassembly is used or produced.
- No game-specific hacks.
- No implementation is provided by this document. Helpers remain `UNIMPL` until
  slice S1 (ABI) lands and a failing-first test pins each behaviour.
