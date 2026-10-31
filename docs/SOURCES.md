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
| S1 | _add the first public source_ | 1 | _scope_ | _citation URL_ |
| S2 | ps2tek — "Documentation on PS2 internals" (psi-rockin.github.io/ps2tek), Emotion Engine / COP0 / TLB sections | 1 | EE TLB entry format, COP0 Index/Random/EntryLo0/1/PageMask/Wired/EntryHi, page sizes, exception codes, scratchpad bit | https://psi-rockin.github.io/ps2tek/ |

## Forbidden

No retail BIOS, no firmware dump, no game ROM, no key, no decryption constant
from a leaked document, no disassembly of first-party code.
