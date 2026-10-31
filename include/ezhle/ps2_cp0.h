/* CP0 (coprocessor 0) register numbers, field layout and exception vectors for
 * the EE core.
 *
 * Source class: public MIPS III/IV core architecture documentation and the
 * public EE overview. Only the registers and fields listed here are modelled;
 * anything else is UNKNOWN in this checkout and must return EZHLE_ERR_UNIMPL
 * rather than be guessed. */
#ifndef EZHLE_PS2_CP0_H
#define EZHLE_PS2_CP0_H

#include <stdint.h>

/* CP0 register numbers. */
#define PS2_CP0_BADVADDR 8u
#define PS2_CP0_COUNT 9u
#define PS2_CP0_COMPARE 11u
#define PS2_CP0_STATUS 12u
#define PS2_CP0_CAUSE 13u
#define PS2_CP0_EPC 14u

/* CP0 Status (register 12) fields. */
#define PS2_STATUS_IE (1u << 0)      /* global interrupt enable */
#define PS2_STATUS_EXL (1u << 1)     /* exception level */
#define PS2_STATUS_ERL (1u << 2)     /* error level */
#define PS2_STATUS_KSU (3u << 3)     /* kernel=0, supervisor=1, user=2 */
#define PS2_STATUS_IM (0xFFu << 8)   /* eight interrupt mask bits */
#define PS2_STATUS_BEV (1u << 22)    /* boot exception vectors */
#define PS2_STATUS_CU0 (1u << 28)    /* coprocessor 0 usable */

/* CP0 Cause (register 13) fields. */
#define PS2_CAUSE_EXCCODE_SHIFT 2u
#define PS2_CAUSE_IP_SHIFT 8u
#define PS2_CAUSE_EXCCODE (0x1Fu << PS2_CAUSE_EXCCODE_SHIFT)
#define PS2_CAUSE_IP (0xFFu << PS2_CAUSE_IP_SHIFT)
#define PS2_CAUSE_BD (1u << 31)      /* exception taken in a branch delay slot */

/* Cause.IP line accessors: line 0..7 maps to Cause bit 8..15, and the matching
 * Status.IM mask bit is the same bit number. */
#define PS2_CAUSE_IP_LINE(line) (1u << (PS2_CAUSE_IP_SHIFT + (line)))
#define PS2_STATUS_IM_LINE(line) (1u << (PS2_CAUSE_IP_SHIFT + (line)))

/* Exception codes (Cause.ExcCode). */
#define PS2_EXC_INT 0u   /* interrupt */
#define PS2_EXC_MOD 1u   /* TLB modification */
#define PS2_EXC_TLBL 2u  /* TLB miss on load / instruction fetch */
#define PS2_EXC_TLBS 3u  /* TLB miss on store */
#define PS2_EXC_ADEL 4u  /* address error on load / instruction fetch */
#define PS2_EXC_ADES 5u  /* address error on store */
#define PS2_EXC_SYS 6u   /* syscall */
#define PS2_EXC_BP 7u    /* breakpoint */
#define PS2_EXC_RI 10u   /* reserved instruction */
#define PS2_EXC_CPU 11u  /* coprocessor unusable */
#define PS2_EXC_OV 12u   /* arithmetic overflow */
#define PS2_EXC_TR 13u   /* trap */
#define PS2_EXC_FPE 15u  /* floating-point exception */

/* Exception vector addresses. */
#define PS2_VECTOR_TLB_REFILL_BEV1 0xBFC00200u
#define PS2_VECTOR_TLB_REFILL_BEV0 0x80000000u
#define PS2_VECTOR_GENERAL_BEV1 0xBFC00380u
#define PS2_VECTOR_GENERAL_BEV0 0x80000180u

/* Reset: the core starts in the boot segment with boot vectors and error level
 * set, and interrupts disabled. */
#define PS2_RESET_PC 0xBFC00000u

/* The EE interrupt controller drives core interrupt line 2. */
#define PS2_CPU_IP_INTC 2u

#endif /* EZHLE_PS2_CP0_H */
