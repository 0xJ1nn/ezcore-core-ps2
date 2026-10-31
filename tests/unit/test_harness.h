/* Minimal unit-test harness for this repository.
 * No allocation, no framework, no global mutable state outside the test binary. */
#ifndef EZHLE_TESTS_UNIT_TEST_HARNESS_H
#define EZHLE_TESTS_UNIT_TEST_HARNESS_H

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

static int ezhle_checks_run;
static int ezhle_checks_failed;

#define CHECK(cond)                                                   \
    do {                                                              \
        ezhle_checks_run++;                                           \
        if (!(cond)) {                                                \
            ezhle_checks_failed++;                                    \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__,   \
                    #cond);                                           \
        }                                                             \
    } while (0)

#define CHECK_EQ_U32(actual, expected)                                        \
    do {                                                                      \
        uint32_t ezhle_a_ = (uint32_t)(actual);                               \
        uint32_t ezhle_e_ = (uint32_t)(expected);                             \
        ezhle_checks_run++;                                                   \
        if (ezhle_a_ != ezhle_e_) {                                           \
            ezhle_checks_failed++;                                            \
            fprintf(stderr,                                                   \
                    "FAIL %s:%d: %s == 0x%08" PRIx32 ", expected 0x%08" PRIx32 \
                    "\n",                                                     \
                    __FILE__, __LINE__, #actual, ezhle_a_, ezhle_e_);         \
        }                                                                     \
    } while (0)

#define TEST_REPORT()                                                      \
    (printf("%d checks, %d failed\n", ezhle_checks_run,                    \
            ezhle_checks_failed),                                          \
     ezhle_checks_failed == 0 ? 0 : 1)

#endif /* EZHLE_TESTS_UNIT_TEST_HARNESS_H */
