#include "ezhle/ezhle_ps2.h"

#include <stddef.h>

ezhle_err_t ps2_cpu_syscall_cycles(unsigned int syscall_id, unsigned int *out_cycles) {
    (void)syscall_id;
    if (out_cycles == NULL) {
        return EZHLE_ERR_INVALID_ARG;
    }
    /* Timing for PS2 EE syscall paths is undocumented in this repo's sources.
     * See docs/UNKNOWNS.md (U-CPU-001). No value is guessed. */
    return EZHLE_ERR_UNIMPL;
}
