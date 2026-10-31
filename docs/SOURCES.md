# Sources — PlayStation 2

Every behaviour implemented here must trace to a public, citable source.
Record the source tier before writing code.

| Tier | Meaning | Allowed |
|---|---|---|
| 0 | Public format/OS documentation, official manuals | yes |
| 1 | Public reverse-engineering notes with citations (e.g. NESdev, psx-spx with caveats) | yes, cite |
| 2 | Permissively licensed emulator source (MIT/BSD/Apache) | readable with attribution; never copy into a differently licensed module |
| 3 | GPL/non-commercial source, BIOS disassembly, leaked SDK | **black-box oracle only; never read into a clean-room implementation** |

## Registered sources

| ID | Source | Tier | Used for | Notes |
|---|---|---|---|---|
| S1 | MIPS IV instruction set architecture, public architecture manual | 0 | CP0 register numbers, Status/Cause field layout, exception code numbers, exception vector addresses, EPC/BD/BadVaddr entry rules, reset state, interrupt delivery gating | cited by document name only; no URL recorded in this checkout |
| S2 | Public EE hardware overview (Emotion Engine core and interrupt controller) | 1 | INTC_STAT (0x00) / INTC_MASK (0x10) offsets, INTC_STAT write-one-to-clear, eleven interrupt sources, controller output on core interrupt line 2 | cited by document name only; per-field confidence noted in `include/ezhle/ps2_cp0.h` and `include/ezhle/ps2_intc.h` |

Source ids share the `S` prefix with the migration slice ids in
`docs/SLICES.md`; the two namespaces are separate. Slice S1, for example, is the
build slice, not the MIPS manual.


## Forbidden

No retail BIOS, no firmware dump, no game ROM, no key, no decryption constant
from a leaked document, no disassembly of first-party code.
