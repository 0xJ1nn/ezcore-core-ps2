#include "ezhle/ezhle_ps2.h"

#include <stdio.h>

static int failures = 0;

#define CHECK(cond, msg) \
    do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", (msg)); failures++; } } while (0)

int main(void) {
    unsigned int cycles = 0;

    /* Undocumented syscall id -> UNIMPL, no crash. */
    CHECK(ps2_cpu_syscall_cycles(0u, &cycles) == EZHLE_ERR_UNIMPL,
          "syscall 0 returns UNIMPL");

    /* NULL guest/pointer -> invalid arg. */
    CHECK(ps2_cpu_syscall_cycles(0u, NULL) == EZHLE_ERR_INVALID_ARG,
          "NULL out_cycles -> INVALID_ARG");

    /* Determinism: two runs agree. */
    ezhle_err_t a = ps2_cpu_syscall_cycles(7u, &cycles);
    ezhle_err_t b = ps2_cpu_syscall_cycles(7u, &cycles);
    CHECK(a == b, "deterministic result across calls");

    if (failures) {
        fprintf(stderr, "%d failure(s)\n", failures);
        return 1;
    }
    printf("test_cpu_syscall_timing: ok\n");
    return 0;
}
