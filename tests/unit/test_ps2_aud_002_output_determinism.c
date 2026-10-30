/* PS2-AUD-002 — Audio output determinism.
 *
 * This test pins the required hook. It cannot compile today because the PS2
 * public ABI header (slice S1 deliverable) does not exist in this repository,
 * and with it the audio output determinism entry point it must export.
 * See docs/PS2-AUD-002-BLOCKER.md.
 *
 * Tests first: this file is intentionally RED until the hook lands.
 */

#include "ezhle/ezhle_ps2.h"

#include <stdio.h>

int main(void)
{
    /* The audio output determinism entry point must be declared by the PS2 ABI
     * header. Its absence is the blocker named in PS2-AUD-002-BLOCKER.md. */
    (void)ezhle_ps2_audio_output_determinism_selftest;
    (void)puts("PS2-AUD-002: audio output determinism hook present");
    return 0;
}
