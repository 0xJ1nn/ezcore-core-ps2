/* ezCORE HLE — PlayStation 2 public ABI (audio output determinism).
 *
 * Clean-room. No firmware, BIOS, ROM, key, or disassembly.
 * Tier 0/1 sources only; see docs/SOURCES.md.
 */

#ifndef EZHLE_PS2_H
#define EZHLE_PS2_H

#include <stddef.h>
#include <stdint.h>

/* Returns 0 on success, non-zero on failure. */
int ezhle_ps2_audio_output_determinism_selftest(void);

#endif /* EZHLE_PS2_H */
