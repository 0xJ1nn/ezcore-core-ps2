/* PS2-CPU-003: CP0 exception entry and interrupt delivery.
 * Behaviours below are the public MIPS III/IV core exception model; every
 * assertion names the documented rule it pins. */
#include <stdbool.h>
#include <stdint.h>

#include "ezhle/ezhle_status.h"
#include "ezhle/ps2_cp0.h"
#include "ezhle/ps2_cpu.h"
#include "test_harness.h"

/* Reset leaves the core in the boot state: BEV=1, ERL=1, IE=0, PC at the boot
 * vector, and no exception history. */
static void test_reset(void)
{
    ps2_cpu cpu;

    cpu.pc = 0xDEADBEEFu;
    cpu.status = 0xFFFFFFFFu;
    cpu.cause = 0xFFFFFFFFu;
    cpu.epc = 0xDEADBEEFu;
    cpu.badvaddr = 0xDEADBEEFu;

    ps2_cpu_reset(&cpu);

    CHECK_EQ_U32(cpu.pc, PS2_RESET_PC);
    CHECK((cpu.status & PS2_STATUS_BEV) != 0u);
    CHECK((cpu.status & PS2_STATUS_ERL) != 0u);
    CHECK((cpu.status & PS2_STATUS_IE) == 0u);
    CHECK((cpu.status & PS2_STATUS_EXL) == 0u);
    CHECK_EQ_U32(cpu.cause, 0u);
    CHECK_EQ_U32(cpu.epc, 0u);
    CHECK_EQ_U32(cpu.badvaddr, 0u);

    /* A NULL state is rejected without touching memory. */
    ps2_cpu_reset(NULL);
    CHECK(1);
}

/* Vector selection: BEV picks the boot or kernel pair; only a TLB refill taken
 * at EXL=0 uses the refill vector, everything else uses the general vector. */
static void test_vectors(void)
{
    ps2_cpu cpu;
    uint32_t vector = 0u;

    ps2_cpu_reset(&cpu);

    cpu.status = PS2_STATUS_BEV;
    CHECK_EQ_U32(ps2_cpu_exception_vector(&cpu, PS2_EXC_INT, &vector), EZHLE_OK);
    CHECK_EQ_U32(vector, PS2_VECTOR_GENERAL_BEV1);
    CHECK_EQ_U32(ps2_cpu_exception_vector(&cpu, PS2_EXC_SYS, &vector), EZHLE_OK);
    CHECK_EQ_U32(vector, PS2_VECTOR_GENERAL_BEV1);
    CHECK_EQ_U32(ps2_cpu_exception_vector(&cpu, PS2_EXC_TLBL, &vector), EZHLE_OK);
    CHECK_EQ_U32(vector, PS2_VECTOR_TLB_REFILL_BEV1);
    CHECK_EQ_U32(ps2_cpu_exception_vector(&cpu, PS2_EXC_TLBS, &vector), EZHLE_OK);
    CHECK_EQ_U32(vector, PS2_VECTOR_TLB_REFILL_BEV1);

    cpu.status = 0u;
    CHECK_EQ_U32(ps2_cpu_exception_vector(&cpu, PS2_EXC_ADEL, &vector), EZHLE_OK);
    CHECK_EQ_U32(vector, PS2_VECTOR_GENERAL_BEV0);
    CHECK_EQ_U32(ps2_cpu_exception_vector(&cpu, PS2_EXC_TLBL, &vector), EZHLE_OK);
    CHECK_EQ_U32(vector, PS2_VECTOR_TLB_REFILL_BEV0);

    /* At EXL=1 a TLB refill must take the general vector. */
    cpu.status = PS2_STATUS_EXL;
    CHECK_EQ_U32(ps2_cpu_exception_vector(&cpu, PS2_EXC_TLBS, &vector), EZHLE_OK);
    CHECK_EQ_U32(vector, PS2_VECTOR_GENERAL_BEV0);

    /* Argument checks. */
    CHECK_EQ_U32(ps2_cpu_exception_vector(NULL, PS2_EXC_INT, &vector),
                 EZHLE_ERR_INVAL);
    cpu.status = 0u;
    CHECK_EQ_U32(ps2_cpu_exception_vector(&cpu, PS2_EXC_INT, NULL),
                 EZHLE_ERR_INVAL);
    /* Machine check and unassigned codes are not modelled here. */
    vector = 0xA5A5A5A5u;
    CHECK_EQ_U32(ps2_cpu_exception_vector(&cpu, 24u, &vector), EZHLE_ERR_UNIMPL);
    CHECK_EQ_U32(vector, 0xA5A5A5A5u);
    CHECK_EQ_U32(ps2_cpu_exception_vector(&cpu, 9u, &vector), EZHLE_ERR_UNIMPL);
    CHECK_EQ_U32(ps2_cpu_exception_vector(&cpu, 31u, &vector), EZHLE_ERR_UNIMPL);
}

/* A synchronous exception from normal context records the faulting PC in EPC,
 * clears BD, writes ExcCode, sets EXL and enters the vector. */
static void test_raise_from_normal_context(void)
{
    ps2_cpu cpu;

    ps2_cpu_reset(&cpu);
    cpu.status = PS2_STATUS_BEV;
    cpu.pc = 0x00100000u;

    CHECK_EQ_U32(ps2_cpu_raise_exception(&cpu, PS2_EXC_SYS, cpu.pc, false, 0u,
                                         false),
                 EZHLE_OK);
    CHECK_EQ_U32(cpu.epc, 0x00100000u);
    CHECK_EQ_U32(cpu.pc, PS2_VECTOR_GENERAL_BEV1);
    CHECK((cpu.status & PS2_STATUS_EXL) != 0u);
    CHECK((cpu.cause & PS2_CAUSE_BD) == 0u);
    CHECK_EQ_U32((cpu.cause & PS2_CAUSE_EXCCODE) >> PS2_CAUSE_EXCCODE_SHIFT,
                 PS2_EXC_SYS);
    /* SYS carries no faulting address, so BadVaddr stays untouched. */
    CHECK_EQ_U32(cpu.badvaddr, 0u);
}

/* An exception in a branch delay slot records the branch in EPC and sets BD. */
static void test_raise_in_delay_slot(void)
{
    ps2_cpu cpu;

    ps2_cpu_reset(&cpu);
    cpu.status = PS2_STATUS_BEV;
    cpu.pc = 0x00100004u;

    CHECK_EQ_U32(ps2_cpu_raise_exception(&cpu, PS2_EXC_BP, cpu.pc, true, 0u,
                                         false),
                 EZHLE_OK);
    CHECK_EQ_U32(cpu.epc, 0x00100000u);
    CHECK((cpu.cause & PS2_CAUSE_BD) != 0u);
    CHECK_EQ_U32((cpu.cause & PS2_CAUSE_EXCCODE) >> PS2_CAUSE_EXCCODE_SHIFT,
                 PS2_EXC_BP);
    CHECK_EQ_U32(cpu.pc, PS2_VECTOR_GENERAL_BEV1);
}

/* Address exceptions record the faulting address in BadVaddr at EXL=0. */
static void test_raise_address_exception(void)
{
    ps2_cpu cpu;

    ps2_cpu_reset(&cpu);
    cpu.status = 0u;
    cpu.pc = 0x00001000u;

    CHECK_EQ_U32(ps2_cpu_raise_exception(&cpu, PS2_EXC_ADEL, cpu.pc, false,
                                         0x30u, true),
                 EZHLE_OK);
    CHECK_EQ_U32(cpu.badvaddr, 0x30u);
    CHECK_EQ_U32((cpu.cause & PS2_CAUSE_EXCCODE) >> PS2_CAUSE_EXCCODE_SHIFT,
                 PS2_EXC_ADEL);
    CHECK_EQ_U32(cpu.pc, PS2_VECTOR_GENERAL_BEV0);
}

/* With EXL already set, a second exception keeps EPC, BD and BadVaddr from the
 * first one and only replaces ExcCode. */
static void test_nested_exception_keeps_epc(void)
{
    ps2_cpu cpu;

    ps2_cpu_reset(&cpu);
    cpu.status = 0u;
    cpu.pc = 0x80001000u;

    CHECK_EQ_U32(ps2_cpu_raise_exception(&cpu, PS2_EXC_ADEL, cpu.pc, false,
                                         0x30u, true),
                 EZHLE_OK);
    CHECK_EQ_U32(cpu.epc, 0x80001000u);
    CHECK_EQ_U32(cpu.badvaddr, 0x30u);
    CHECK_EQ_U32(cpu.pc, PS2_VECTOR_GENERAL_BEV0);

    CHECK_EQ_U32(ps2_cpu_raise_exception(&cpu, PS2_EXC_ADES, cpu.pc, false,
                                         0x40u, true),
                 EZHLE_OK);
    CHECK_EQ_U32(cpu.epc, 0x80001000u);
    CHECK_EQ_U32(cpu.badvaddr, 0x30u);
    CHECK((cpu.cause & PS2_CAUSE_BD) == 0u);
    CHECK_EQ_U32((cpu.cause & PS2_CAUSE_EXCCODE) >> PS2_CAUSE_EXCCODE_SHIFT,
                 PS2_EXC_ADES);
    CHECK_EQ_U32(cpu.pc, PS2_VECTOR_GENERAL_BEV0);
}

/* Rejected inputs leave the state untouched. */
static void test_raise_rejects_bad_input(void)
{
    ps2_cpu cpu;
    ps2_cpu snapshot;

    ps2_cpu_reset(&cpu);
    cpu.status = PS2_STATUS_BEV;
    cpu.pc = 0x00100000u;
    snapshot = cpu;

    /* Undocumented exception codes. */
    CHECK_EQ_U32(ps2_cpu_raise_exception(&cpu, 24u, cpu.pc, false, 0u, false),
                 EZHLE_ERR_UNIMPL);
    CHECK_EQ_U32(ps2_cpu_raise_exception(&cpu, 9u, cpu.pc, false, 0u, false),
                 EZHLE_ERR_UNIMPL);
    CHECK_EQ_U32(ps2_cpu_raise_exception(&cpu, 31u, cpu.pc, false, 0u, false),
                 EZHLE_ERR_UNIMPL);

    /* Address exception without an address, and vice versa. */
    CHECK_EQ_U32(ps2_cpu_raise_exception(&cpu, PS2_EXC_ADEL, cpu.pc, false, 0u,
                                         false),
                 EZHLE_ERR_INVAL);
    CHECK_EQ_U32(ps2_cpu_raise_exception(&cpu, PS2_EXC_INT, cpu.pc, false,
                                         0x10u, true),
                 EZHLE_ERR_INVAL);

    /* A delay slot cannot precede address zero. */
    CHECK_EQ_U32(ps2_cpu_raise_exception(&cpu, PS2_EXC_BP, 2u, true, 0u, false),
                 EZHLE_ERR_INVAL);

    /* NULL state. */
    CHECK_EQ_U32(ps2_cpu_raise_exception(NULL, PS2_EXC_BP, 0u, false, 0u, false),
                 EZHLE_ERR_INVAL);

    CHECK_EQ_U32(cpu.pc, snapshot.pc);
    CHECK_EQ_U32(cpu.status, snapshot.status);
    CHECK_EQ_U32(cpu.cause, snapshot.cause);
    CHECK_EQ_U32(cpu.epc, snapshot.epc);
    CHECK_EQ_U32(cpu.badvaddr, snapshot.badvaddr);
}

/* A maskable interrupt is pending only when IE=1, EXL=0, ERL=0 and a Cause.IP
 * line is enabled by Status.IM. */
static void test_interrupt_pending(void)
{
    ps2_cpu cpu;

    ps2_cpu_reset(&cpu);
    cpu.status = PS2_STATUS_IM_LINE(3) | PS2_STATUS_IE;
    CHECK_EQ_U32(ps2_cpu_interrupt_pending(&cpu), 0u);

    CHECK_EQ_U32(ps2_cpu_set_ip_line(&cpu, 3, true), EZHLE_OK);
    CHECK((cpu.cause & PS2_CAUSE_IP_LINE(3)) != 0u);
    CHECK_EQ_U32(ps2_cpu_interrupt_pending(&cpu), 1u);

    /* A line that is set but masked out does not count. */
    cpu.status = PS2_STATUS_IM_LINE(4) | PS2_STATUS_IE;
    CHECK_EQ_U32(ps2_cpu_interrupt_pending(&cpu), 0u);

    /* EXL, ERL and IE=0 each block delivery. */
    cpu.status = PS2_STATUS_IM_LINE(3) | PS2_STATUS_IE | PS2_STATUS_EXL;
    CHECK_EQ_U32(ps2_cpu_interrupt_pending(&cpu), 0u);
    cpu.status = PS2_STATUS_IM_LINE(3) | PS2_STATUS_IE | PS2_STATUS_ERL;
    CHECK_EQ_U32(ps2_cpu_interrupt_pending(&cpu), 0u);
    cpu.status = PS2_STATUS_IM_LINE(3);
    CHECK_EQ_U32(ps2_cpu_interrupt_pending(&cpu), 0u);

    /* Line index checks. */
    CHECK_EQ_U32(ps2_cpu_set_ip_line(&cpu, 7, false), EZHLE_OK);
    CHECK_EQ_U32(ps2_cpu_set_ip_line(&cpu, 8, true), EZHLE_ERR_INVAL);
    CHECK_EQ_U32(ps2_cpu_set_ip_line(NULL, 0, true), EZHLE_ERR_INVAL);

    CHECK_EQ_U32(ps2_cpu_set_ip_line(&cpu, 3, false), EZHLE_OK);
    CHECK((cpu.cause & PS2_CAUSE_IP_LINE(3)) == 0u);
    CHECK_EQ_U32(ps2_cpu_interrupt_pending(NULL), 0u);
}

/* Taking an interrupt performs a normal exception entry with code Int at the
 * interrupted PC, and only when one is pending. */
static void test_take_interrupt(void)
{
    ps2_cpu cpu;

    ps2_cpu_reset(&cpu);
    cpu.status = PS2_STATUS_IM_LINE(2) | PS2_STATUS_IE;
    cpu.pc = 0x00123456u;

    CHECK_EQ_U32(ps2_cpu_take_interrupt(&cpu), 0u);
    CHECK_EQ_U32(cpu.pc, 0x00123456u);
    CHECK_EQ_U32(cpu.epc, 0u);

    CHECK_EQ_U32(ps2_cpu_set_ip_line(&cpu, 2, true), EZHLE_OK);
    CHECK_EQ_U32(ps2_cpu_take_interrupt(&cpu), 1u);
    CHECK_EQ_U32(cpu.pc, PS2_VECTOR_GENERAL_BEV0);
    CHECK_EQ_U32(cpu.epc, 0x00123456u);
    CHECK((cpu.status & PS2_STATUS_EXL) != 0u);
    CHECK((cpu.cause & PS2_CAUSE_BD) == 0u);
    CHECK_EQ_U32((cpu.cause & PS2_CAUSE_EXCCODE) >> PS2_CAUSE_EXCCODE_SHIFT,
                 PS2_EXC_INT);

    /* EXL is now set, so the same line cannot be taken again. */
    CHECK_EQ_U32(ps2_cpu_take_interrupt(&cpu), 0u);
    CHECK_EQ_U32(ps2_cpu_take_interrupt(NULL), 0u);
}

int main(void)
{
    test_reset();
    test_vectors();
    test_raise_from_normal_context();
    test_raise_in_delay_slot();
    test_raise_address_exception();
    test_nested_exception_keeps_epc();
    test_raise_rejects_bad_input();
    test_interrupt_pending();
    test_take_interrupt();
    return TEST_REPORT();
}
