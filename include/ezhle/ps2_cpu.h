/* EE core exception state and entry.
 *
 * This module owns the CP0 exception model only: the register fields that an
 * exception writes, vector selection, and interrupt delivery gating. It does
 * not decode or execute instructions and it does not fetch from the vector;
 * those belong to the core and bus modules, which do not exist in this
 * repository yet (see handoff.md, "Interface boundary").
 *
 * No allocation, no global mutable state: all state lives in ps2_cpu. */
#ifndef EZHLE_PS2_CPU_H
#define EZHLE_PS2_CPU_H

#include <stdbool.h>
#include <stdint.h>

#include "ezhle/ezhle_status.h"
#include "ezhle/ps2_cp0.h"

typedef struct ps2_cpu {
    uint32_t pc;       /* next instruction to execute */
    uint32_t status;   /* CP0 Status */
    uint32_t cause;    /* CP0 Cause */
    uint32_t epc;      /* CP0 EPC */
    uint32_t badvaddr; /* CP0 BadVaddr */
} ps2_cpu;

/* Put the core in its reset state: PS2_RESET_PC, BEV and ERL set, IE clear, and
 * Cause, EPC and BadVaddr cleared. */
void ps2_cpu_reset(ps2_cpu *cpu);

/* Vector an exception of exc_code would take in the current state. Returns
 * EZHLE_ERR_UNIMPL for an exception code this module does not model. */
int ps2_cpu_exception_vector(const ps2_cpu *cpu, uint32_t exc_code,
                             uint32_t *vector);

/* Enter an exception.
 *
 * pc_of_instruction is the PC of the instruction that raised it and
 * in_delay_slot says whether that instruction was a branch delay slot. For the
 * address-carrying codes (MOD, TLBL, TLBS, ADEL, ADES) fault_addr must be
 * supplied and fault_addr_valid set; for every other code fault_addr_valid must
 * be clear. Rejected input returns EZHLE_ERR_INVAL and leaves the state
 * untouched; an unmodelled code returns EZHLE_ERR_UNIMPL, also untouched. */
int ps2_cpu_raise_exception(ps2_cpu *cpu, uint32_t exc_code,
                            uint32_t pc_of_instruction, bool in_delay_slot,
                            uint32_t fault_addr, bool fault_addr_valid);

/* Drive one Cause.IP line (0..7) high or low, as an external interrupt source
 * would. Returns EZHLE_ERR_INVAL for a line number outside 0..7. */
int ps2_cpu_set_ip_line(ps2_cpu *cpu, uint32_t line, bool level);

/* 1 when a maskable interrupt may be delivered: IE=1, EXL=0, ERL=0 and at least
 * one Cause.IP line enabled by Status.IM. 0 otherwise. */
int ps2_cpu_interrupt_pending(const ps2_cpu *cpu);

/* Take a pending interrupt as exception code INT at the current PC. Returns 1
 * when an interrupt was taken, 0 when none was pending. */
int ps2_cpu_take_interrupt(ps2_cpu *cpu);

#endif /* EZHLE_PS2_CPU_H */
