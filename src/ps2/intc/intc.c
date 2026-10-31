/* PS2-CPU-003: EE interrupt controller registers.
 *
 * Model source: public EE overview documentation. INTC_STAT (0x00) holds the
 * pending sources and is write-one-to-clear; INTC_MASK (0x10) holds the
 * per-source enables. Eleven sources exist, so bits 0..10 are modelled and the
 * rest of each word reads as zero. Any other offset is not modelled here and
 * returns EZHLE_ERR_UNIMPL.
 *
 * No allocation, no global mutable state: all state lives in the caller's
 * ps2_intc. */
#include <stddef.h>

#include "ezhle/ps2_intc.h"

void ps2_intc_reset(ps2_intc *intc)
{
    if (intc == NULL) {
        return;
    }

    intc->stat = 0u;
    intc->mask = 0u;
}

int ps2_intc_raise(ps2_intc *intc, uint32_t sources)
{
    if (intc == NULL) {
        return EZHLE_ERR_INVAL;
    }

    intc->stat |= sources & PS2_INTC_SOURCE_MASK;
    return EZHLE_OK;
}

int ps2_intc_read(const ps2_intc *intc, uint32_t offset, uint32_t *value)
{
    if (intc == NULL || value == NULL) {
        return EZHLE_ERR_INVAL;
    }

    switch (offset) {
    case PS2_INTC_STAT:
        *value = intc->stat & PS2_INTC_SOURCE_MASK;
        return EZHLE_OK;
    case PS2_INTC_MASK:
        *value = intc->mask & PS2_INTC_SOURCE_MASK;
        return EZHLE_OK;
    default:
        return EZHLE_ERR_UNIMPL;
    }
}

int ps2_intc_write(ps2_intc *intc, uint32_t offset, uint32_t value)
{
    if (intc == NULL) {
        return EZHLE_ERR_INVAL;
    }

    switch (offset) {
    case PS2_INTC_STAT:
        /* Writing one clears that source; writing zero leaves it pending. */
        intc->stat &= ~(value & PS2_INTC_SOURCE_MASK);
        return EZHLE_OK;
    case PS2_INTC_MASK:
        intc->mask = value & PS2_INTC_SOURCE_MASK;
        return EZHLE_OK;
    default:
        return EZHLE_ERR_UNIMPL;
    }
}

int ps2_intc_irq(const ps2_intc *intc)
{
    if (intc == NULL) {
        return 0;
    }

    return ((intc->stat & intc->mask & PS2_INTC_SOURCE_MASK) != 0u) ? 1 : 0;
}
