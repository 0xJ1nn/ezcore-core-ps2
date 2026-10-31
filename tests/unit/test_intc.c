/* PS2-CPU-003: the EE interrupt controller registers and its line into the core. */
#include <stdbool.h>
#include <stdint.h>

#include "ezhle/ezhle_status.h"
#include "ezhle/ps2_cp0.h"
#include "ezhle/ps2_cpu.h"
#include "ezhle/ps2_intc.h"
#include "test_harness.h"

static void test_reset(void)
{
    ps2_intc intc;

    intc.stat = 0xFFFFFFFFu;
    intc.mask = 0xFFFFFFFFu;
    ps2_intc_reset(&intc);

    CHECK_EQ_U32(intc.stat, 0u);
    CHECK_EQ_U32(intc.mask, 0u);
    CHECK_EQ_U32(ps2_intc_irq(&intc), 0u);

    ps2_intc_reset(NULL);
    CHECK(1);
}

/* A source is raised by its device and only reaches the core when it is also
 * unmasked. */
static void test_irq_needs_stat_and_mask(void)
{
    ps2_intc intc;
    uint32_t value = 0u;

    ps2_intc_reset(&intc);

    CHECK_EQ_U32(ps2_intc_raise(&intc, 1u << 0), EZHLE_OK);
    CHECK_EQ_U32(ps2_intc_irq(&intc), 0u);
    CHECK_EQ_U32(ps2_intc_read(&intc, PS2_INTC_STAT, &value), EZHLE_OK);
    CHECK_EQ_U32(value, 1u << 0);

    CHECK_EQ_U32(ps2_intc_write(&intc, PS2_INTC_MASK, 1u << 0), EZHLE_OK);
    CHECK_EQ_U32(ps2_intc_irq(&intc), 1u);

    /* Source 3 pending but not masked in: the line stays down. Clear source 0
     * first so the only pending source is the masked-out one. */
    CHECK_EQ_U32(ps2_intc_write(&intc, PS2_INTC_STAT, 1u << 0), EZHLE_OK);
    CHECK_EQ_U32(ps2_intc_read(&intc, PS2_INTC_STAT, &value), EZHLE_OK);
    CHECK_EQ_U32(value, 0u);
    CHECK_EQ_U32(ps2_intc_write(&intc, PS2_INTC_MASK, 0u), EZHLE_OK);
    CHECK_EQ_U32(ps2_intc_raise(&intc, 1u << 3), EZHLE_OK);
    CHECK_EQ_U32(ps2_intc_irq(&intc), 0u);
    CHECK_EQ_U32(ps2_intc_write(&intc, PS2_INTC_MASK, (1u << 0) | (1u << 4)),
                 EZHLE_OK);
    CHECK_EQ_U32(ps2_intc_irq(&intc), 0u);
    CHECK_EQ_U32(ps2_intc_write(&intc, PS2_INTC_MASK, 1u << 3), EZHLE_OK);
    CHECK_EQ_U32(ps2_intc_irq(&intc), 1u);
    CHECK_EQ_U32(ps2_intc_raise(NULL, 1u), EZHLE_ERR_INVAL);
}

/* INTC_STAT is write-one-to-clear: writing 0 keeps a pending source. */
static void test_stat_is_write_one_to_clear(void)
{
    ps2_intc intc;
    uint32_t value = 0u;

    ps2_intc_reset(&intc);
    CHECK_EQ_U32(ps2_intc_write(&intc, PS2_INTC_MASK, 0x7FFu), EZHLE_OK);
    CHECK_EQ_U32(ps2_intc_raise(&intc, 0x07Fu), EZHLE_OK);
    CHECK_EQ_U32(ps2_intc_irq(&intc), 1u);

    /* Writing zero clears nothing. */
    CHECK_EQ_U32(ps2_intc_write(&intc, PS2_INTC_STAT, 0u), EZHLE_OK);
    CHECK_EQ_U32(ps2_intc_read(&intc, PS2_INTC_STAT, &value), EZHLE_OK);
    CHECK_EQ_U32(value, 0x07Fu);

    /* Clearing the low bits leaves the high ones pending. */
    CHECK_EQ_U32(ps2_intc_write(&intc, PS2_INTC_STAT, 0x03Fu), EZHLE_OK);
    CHECK_EQ_U32(ps2_intc_read(&intc, PS2_INTC_STAT, &value), EZHLE_OK);
    CHECK_EQ_U32(value, 0x040u);
    CHECK_EQ_U32(ps2_intc_irq(&intc), 1u);

    CHECK_EQ_U32(ps2_intc_write(&intc, PS2_INTC_STAT, 0x040u), EZHLE_OK);
    CHECK_EQ_U32(ps2_intc_read(&intc, PS2_INTC_STAT, &value), EZHLE_OK);
    CHECK_EQ_U32(value, 0u);
    CHECK_EQ_U32(ps2_intc_irq(&intc), 0u);
}

/* Only the eleven documented sources exist; the rest of the word reads as 0 and
 * ignores writes and raises. */
static void test_unimplemented_bits_read_zero(void)
{
    ps2_intc intc;
    uint32_t value = 0u;

    ps2_intc_reset(&intc);
    CHECK_EQ_U32(ps2_intc_write(&intc, PS2_INTC_MASK, 0xFFFFFFFFu), EZHLE_OK);
    CHECK_EQ_U32(ps2_intc_raise(&intc, PS2_INTC_SOURCE_MASK), EZHLE_OK);
    CHECK_EQ_U32(ps2_intc_read(&intc, PS2_INTC_MASK, &value), EZHLE_OK);
    CHECK_EQ_U32(value, PS2_INTC_SOURCE_MASK);
    CHECK_EQ_U32(ps2_intc_read(&intc, PS2_INTC_STAT, &value), EZHLE_OK);
    CHECK_EQ_U32(value, PS2_INTC_SOURCE_MASK);
    CHECK_EQ_U32(ps2_intc_irq(&intc), 1u);

    /* Sources above 10 do not exist: a raise naming them changes nothing. */
    CHECK_EQ_U32(ps2_intc_raise(&intc, 0xFFFFF800u), EZHLE_OK);
    CHECK_EQ_U32(ps2_intc_read(&intc, PS2_INTC_STAT, &value), EZHLE_OK);
    CHECK_EQ_U32(value, PS2_INTC_SOURCE_MASK);
}

/* Registers this module does not model, and bad arguments, are refused. */
static void test_unmodelled_offsets(void)
{
    ps2_intc intc;
    uint32_t value = 0xA5A5A5A5u;

    ps2_intc_reset(&intc);
    CHECK_EQ_U32(ps2_intc_write(&intc, PS2_INTC_MASK, 1u), EZHLE_OK);

    CHECK_EQ_U32(ps2_intc_read(&intc, 0x04u, &value), EZHLE_ERR_UNIMPL);
    CHECK_EQ_U32(value, 0xA5A5A5A5u);
    CHECK_EQ_U32(ps2_intc_write(&intc, 0x20u, 1u), EZHLE_ERR_UNIMPL);
    CHECK_EQ_U32(intc.mask, 1u);
    CHECK_EQ_U32(intc.stat, 0u);

    CHECK_EQ_U32(ps2_intc_read(NULL, PS2_INTC_STAT, &value), EZHLE_ERR_INVAL);
    CHECK_EQ_U32(ps2_intc_read(&intc, PS2_INTC_STAT, NULL), EZHLE_ERR_INVAL);
    CHECK_EQ_U32(ps2_intc_write(NULL, PS2_INTC_STAT, 1u), EZHLE_ERR_INVAL);
    CHECK_EQ_U32(ps2_intc_irq(NULL), 0u);
}

/* The controller output drives core interrupt line 2, and a taken interrupt
 * reaches the general vector with the pending source still latched. */
static void test_intc_line_reaches_core(void)
{
    ps2_intc intc;
    ps2_cpu cpu;

    ps2_intc_reset(&intc);
    ps2_cpu_reset(&cpu);

    /* Kernel mode, BEV=0, line 2 unmasked, interrupts enabled. */
    cpu.status = PS2_STATUS_IM_LINE(PS2_CPU_IP_INTC) | PS2_STATUS_IE;
    cpu.pc = 0x00100000u;

    CHECK_EQ_U32(ps2_cpu_set_ip_line(&cpu, PS2_CPU_IP_INTC, ps2_intc_irq(&intc)),
                 EZHLE_OK);
    CHECK_EQ_U32(ps2_cpu_interrupt_pending(&cpu), 0u);

    CHECK_EQ_U32(ps2_intc_write(&intc, PS2_INTC_MASK, 1u << 5), EZHLE_OK);
    CHECK_EQ_U32(ps2_intc_raise(&intc, 1u << 5), EZHLE_OK);
    CHECK_EQ_U32(ps2_cpu_set_ip_line(&cpu, PS2_CPU_IP_INTC, ps2_intc_irq(&intc)),
                 EZHLE_OK);
    CHECK_EQ_U32(ps2_cpu_interrupt_pending(&cpu), 1u);

    CHECK_EQ_U32(ps2_cpu_take_interrupt(&cpu), 1u);
    CHECK_EQ_U32(cpu.pc, PS2_VECTOR_GENERAL_BEV0);
    CHECK_EQ_U32(cpu.epc, 0x00100000u);
    CHECK_EQ_U32((cpu.cause & PS2_CAUSE_EXCCODE) >> PS2_CAUSE_EXCCODE_SHIFT,
                 PS2_EXC_INT);

    /* The handler acknowledges the source, which drops the core line; the
     * handler's own EXL still blocks re-entry until it returns. */
    CHECK_EQ_U32(ps2_intc_write(&intc, PS2_INTC_STAT, 1u << 5), EZHLE_OK);
    CHECK_EQ_U32(ps2_intc_irq(&intc), 0u);
    CHECK_EQ_U32(ps2_cpu_set_ip_line(&cpu, PS2_CPU_IP_INTC, ps2_intc_irq(&intc)),
                 EZHLE_OK);
    CHECK_EQ_U32(ps2_cpu_interrupt_pending(&cpu), 0u);
}

int main(void)
{
    test_reset();
    test_irq_needs_stat_and_mask();
    test_stat_is_write_one_to_clear();
    test_unimplemented_bits_read_zero();
    test_unmodelled_offsets();
    test_intc_line_reaches_core();
    return TEST_REPORT();
}
