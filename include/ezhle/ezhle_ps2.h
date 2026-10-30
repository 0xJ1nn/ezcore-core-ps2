#ifndef EZHLE_PS2_H
#define EZHLE_PS2_H

#include "abi.h"

/*
 * CPU timing for syscall paths.
 *
 * Returns the cycle cost of the syscall path identified by `syscall_id`.
 * Undocumented syscall ids return EZHLE_ERR_UNIMPL (see docs/UNKNOWNS.md).
 * `out_cycles` must be non-NULL; otherwise EZHLE_ERR_INVALID_ARG.
 */
ezhle_err_t ps2_cpu_syscall_cycles(unsigned int syscall_id,
                                   unsigned int *out_cycles);

#endif /* EZHLE_PS2_H */
