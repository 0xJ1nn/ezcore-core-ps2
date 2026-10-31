/* tests/unit/test_ps2_bio_009.c
 *
 * PS2-BIO-009 — "Alarms / cache ops / TLB helpers"
 *
 * RED test: this repository's PS2 HLE interface does not exist yet.
 * Slices S1 (shared ABI + build) and S2 (syscall dispatcher) have not
 * landed, so there is no public header, no exported entry table, and no
 * dispatcher to reach any alarm/cache/TLB service. This file is expected
 * to FAIL to compile until those slices land. It is intentionally not
 * edited to pass; see docs/PS2-BIO-009-BLOCKER.md.
 *
 * Tier 0/1 sources: none registered in docs/SOURCES.md for this behaviour
 * yet (placeholder row only). Any alarm/cache/TLB semantic not backed by a
 * cited source is UNKNOWN and must return EZHLE_ERR_UNIMPL.
 */

#include "ezhle/ezhle_ps2.h"   /* missing hook: include/ezhle/ezhle_ps2.h (slice S1) */

int main(void)
{
    /* No PS2 HLE entry point for alarms / cache ops / TLB helpers is
       reachable in this tree. */
    return 0;
}
