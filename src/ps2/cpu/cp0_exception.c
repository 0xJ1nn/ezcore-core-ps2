/* PS2-CPU-003: EE CP0 exception entry and interrupt delivery.
 *
 * Model source: public MIPS III/IV core architecture documentation. Only the
 * documented fields are written; every unmodelled exception code is reported as
 * EZHLE_ERR_UNIMPL and leaves the state untouched.
 *
 * No allocation, no global mutable state, no I/O: every entry point is a pure
 * state transition over the caller's ps2_cpu. */
#include <stddef.h>

#include "ezhle/ps2_cpu.h"

/* Exception codes this module models. Watch (23) and machine check (24) change
 * the entry path (ErrorEPC / ERL) and are UNKNOWN for the EE in this checkout,
 * so they are refused rather than approximated. */
static bool exc_code_supported(uint32_t exc_code)
{
    switch (exc_code) {
    case PS2_EXC_INT:
    case PS2_EXC_MOD:
    case PS2_EXC_TLBL:
    case PS2_EXC_TLBS:
    case PS2_EXC_ADEL:
    case PS2_EXC_ADES:
    case PS2_EXC_SYS:
    case PS2_EXC_BP:
    case PS2_EXC_RI:
    case PS2_EXC_CPU:
    case PS2_EXC_OV:
    case PS2_EXC_TR:
    case PS2_EXC_FPE:
        return true;
    default:
        return false;
    }
}

/* Codes that record the faulting address in BadVaddr. */
static bool exc_code_has_address(uint32_t exc_code)
{
    switch (exc_code) {
    case PS2_EXC_MOD:
    case PS2_EXC_TLBL:
    case PS2_EXC_TLBS:
    case PS2_EXC_ADEL:
    case PS2_EXC_ADES:
        return true;
    default:
        return false;
    }
}

static bool exc_code_is_tlb_refill(uint32_t exc_code)
{
    return exc_code == PS2_EXC_TLBL || exc_code == PS2_EXC_TLBS;
}

void ps2_cpu_reset(ps2_cpu *cpu)
{
    if (cpu == NULL) {
        return;
    }

    cpu->pc = PS2_RESET_PC;
    cpu->status = PS2_STATUS_BEV | PS2_STATUS_ERL;
    cpu->cause = 0u;
    cpu->epc = 0u;
    cpu->badvaddr = 0u;
}

int ps2_cpu_exception_vector(const ps2_cpu *cpu, uint32_t exc_code,
                             uint32_t *vector)
{
    bool bev;
    bool exl;

    if (cpu == NULL || vector == NULL) {
        return EZHLE_ERR_INVAL;
    }
    if (!exc_code_supported(exc_code)) {
        return EZHLE_ERR_UNIMPL;
    }

    bev = (cpu->status & PS2_STATUS_BEV) != 0u;
    exl = (cpu->status & PS2_STATUS_EXL) != 0u;

    /* A TLB refill uses its own vector only when taken at EXL=0; every other
     * exception, and a refill at EXL=1, uses the general vector. */
    if (!exl && exc_code_is_tlb_refill(exc_code)) {
        *vector = bev ? PS2_VECTOR_TLB_REFILL_BEV1 : PS2_VECTOR_TLB_REFILL_BEV0;
    } else {
        *vector = bev ? PS2_VECTOR_GENERAL_BEV1 : PS2_VECTOR_GENERAL_BEV0;
    }
    return EZHLE_OK;
}

int ps2_cpu_raise_exception(ps2_cpu *cpu, uint32_t exc_code,
                            uint32_t pc_of_instruction, bool in_delay_slot,
                            uint32_t fault_addr, bool fault_addr_valid)
{
    uint32_t vector = 0u;
    int rc;

    if (cpu == NULL) {
        return EZHLE_ERR_INVAL;
    }
    if (!exc_code_supported(exc_code)) {
        return EZHLE_ERR_UNIMPL;
    }
    /* The address-carrying codes require an address; the others must not claim
     * one, so a caller cannot silently leave BadVaddr stale. */
    if (exc_code_has_address(exc_code) != fault_addr_valid) {
        return EZHLE_ERR_INVAL;
    }
    if (in_delay_slot && pc_of_instruction < 4u) {
        return EZHLE_ERR_INVAL;
    }

    /* The vector depends on the state before the exception is entered. */
    rc = ps2_cpu_exception_vector(cpu, exc_code, &vector);
    if (rc != EZHLE_OK) {
        return rc;
    }

    /* EPC, BD and BadVaddr are written only when the core was not already at
     * exception level, so a nested exception preserves the first fault. */
    if ((cpu->status & PS2_STATUS_EXL) == 0u) {
        cpu->epc = in_delay_slot ? pc_of_instruction - 4u : pc_of_instruction;
        if (in_delay_slot) {
            cpu->cause |= PS2_CAUSE_BD;
        } else {
            cpu->cause &= ~PS2_CAUSE_BD;
        }
        if (fault_addr_valid) {
            cpu->badvaddr = fault_addr;
        }
    }

    cpu->cause = (cpu->cause & ~PS2_CAUSE_EXCCODE) |
                 ((exc_code << PS2_CAUSE_EXCCODE_SHIFT) & PS2_CAUSE_EXCCODE);
    cpu->status |= PS2_STATUS_EXL;
    cpu->pc = vector;

    return EZHLE_OK;
}

int ps2_cpu_set_ip_line(ps2_cpu *cpu, uint32_t line, bool level)
{
    uint32_t bit;

    if (cpu == NULL || line > 7u) {
        return EZHLE_ERR_INVAL;
    }

    bit = PS2_CAUSE_IP_LINE(line);
    if (level) {
        cpu->cause |= bit;
    } else {
        cpu->cause &= ~bit;
    }
    return EZHLE_OK;
}

int ps2_cpu_interrupt_pending(const ps2_cpu *cpu)
{
    if (cpu == NULL) {
        return 0;
    }
    if ((cpu->status & PS2_STATUS_IE) == 0u) {
        return 0;
    }
    if ((cpu->status & (PS2_STATUS_EXL | PS2_STATUS_ERL)) != 0u) {
        return 0;
    }
    /* Cause.IP and Status.IM share bit positions 8..15. */
    return ((cpu->cause & cpu->status & PS2_STATUS_IM) != 0u) ? 1 : 0;
}

int ps2_cpu_take_interrupt(ps2_cpu *cpu)
{
    int rc;

    if (cpu == NULL || ps2_cpu_interrupt_pending(cpu) == 0) {
        return 0;
    }

    /* A taken interrupt is a normal exception entry with code INT at the
     * instruction that would have executed next. */
    rc = ps2_cpu_raise_exception(cpu, PS2_EXC_INT, cpu->pc, false, 0u, false);
    return (rc == EZHLE_OK) ? 1 : 0;
}
