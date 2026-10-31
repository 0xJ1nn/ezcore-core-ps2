/* EE interrupt controller (INTC) registers.
 *
 * Only the two documented registers are modelled: INTC_STAT (pending sources)
 * and INTC_MASK (per-source enable). Both are word-wide, and only the eleven
 * documented sources (bits 0..10) exist; the remaining bits read as zero and
 * ignore writes. INTC_STAT is write-one-to-clear, so a source is raised by the
 * device through ps2_intc_raise() and cleared by a one-write from software. Any
 * other offset in the page is outside this module and returns
 * EZHLE_ERR_UNIMPL.
 *
 * No allocation, no global mutable state: all state lives in ps2_intc. */
#ifndef EZHLE_PS2_INTC_H
#define EZHLE_PS2_INTC_H

#include <stdint.h>

#include "ezhle/ezhle_status.h"

/* Register offsets within the INTC page. */
#define PS2_INTC_STAT 0x00u
#define PS2_INTC_MASK 0x10u

/* Documented interrupt sources: 11, so bits 0..10. */
#define PS2_INTC_SOURCE_COUNT 11u
#define PS2_INTC_SOURCE_MASK ((1u << PS2_INTC_SOURCE_COUNT) - 1u)

typedef struct ps2_intc {
    uint32_t stat; /* pending sources, write-one-to-clear */
    uint32_t mask; /* per-source enable */
} ps2_intc;

void ps2_intc_reset(ps2_intc *intc);

/* Raise pending sources, as a device line would. Bits above source 10 are not
 * implemented and are ignored. */
int ps2_intc_raise(ps2_intc *intc, uint32_t sources);

/* Read or write one register. EZHLE_ERR_INVAL for a NULL argument,
 * EZHLE_ERR_UNIMPL for an offset this module does not model. */
int ps2_intc_read(const ps2_intc *intc, uint32_t offset, uint32_t *value);
int ps2_intc_write(ps2_intc *intc, uint32_t offset, uint32_t value);

/* The controller output line: 1 when any pending source is also enabled. */
int ps2_intc_irq(const ps2_intc *intc);

#endif /* EZHLE_PS2_INTC_H */
