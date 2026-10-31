/* Vendored from 0xJ1nn/ezcore-core-fds (BSD-2-Clause), include/ezhle/ezhle.h.
 * Shared ABI ezhle-2. See ezcore-core-fds for the canonical copy. */
#ifndef EZHLE_EZHLE_H
#define EZHLE_EZHLE_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EZHLE_ABI_MAGIC 0x455A484Cu
#define EZHLE_ABI_VERSION 2u

enum ezhle_status {
    EZHLE_OK = 0,
    EZHLE_ERR_ARG = -1,
    EZHLE_ERR_NOMEM = -2,
    EZHLE_ERR_UNIMPL = -3,
    EZHLE_ERR_FAULT = -4,
    EZHLE_ERR_STATE = -5
};

typedef struct ezhle_core ezhle_core_t;

typedef struct ezhle_regs {
    uint32_t r[32];
    uint32_t pc;
    uint32_t hi;
    uint32_t lo;
} ezhle_regs_t;

typedef struct ezhle_bus {
    int (*read32)(void *ctx, uint32_t addr, uint32_t *out);
    int (*write32)(void *ctx, uint32_t addr, uint32_t value);
    int (*read_bytes)(void *ctx, uint32_t addr, void *dst, size_t len);
    void (*log)(void *ctx, int level, const char *message);
    void (*trace)(void *ctx, uint32_t nr, const uint32_t *args, size_t argc,
                  int32_t ret);
    void *ctx;
} ezhle_bus_t;

typedef struct ezhle_sched {
    union {
        void (*yield)(void *ctx, int reason, uint32_t thread_id);
        void (*yield_fn)(void *ctx, int reason, uint32_t thread_id);
        void (*on_yield)(void *ctx, int reason, uint32_t thread_id);
        void (*sched_yield)(void *ctx, int reason, uint32_t thread_id);
    };
    union {
        void (*make_ready)(void *ctx, uint32_t thread_id);
        void (*make_ready_fn)(void *ctx, uint32_t thread_id);
        void (*on_make_ready)(void *ctx, uint32_t thread_id);
        void (*sched_make_ready)(void *ctx, uint32_t thread_id);
    };
    union {
        void *ctx;
        void *context;
        void *user;
        void *user_ctx;
        void *user_data;
    };
} ezhle_sched_t;

enum ezhle_sched_reason {
    EZHLE_SCHED_BLOCKED = 0,
    EZHLE_SCHED_SLEEP = 1,
    EZHLE_SCHED_WAIT_SEMA = 2,
    EZHLE_SCHED_WAIT_EVENTFLAG = 3
};

typedef struct ezhle_desc {
    uint32_t magic;
    uint32_t abi_version;
    uint32_t struct_size;
    const char *system;
    const char *build_id;
    const ezhle_bus_t *bus;
    const ezhle_sched_t *sched;
    const ezhle_sched_t *scheduler;
    const ezhle_sched_t *sched_cb;
    const ezhle_sched_t *sched_callbacks;
    const ezhle_sched_t *thread_sched;
    const ezhle_sched_t *sched_ops;
    void (*yield)(void *ctx, int reason, uint32_t thread_id);
    void (*yield_fn)(void *ctx, int reason, uint32_t thread_id);
    void (*on_yield)(void *ctx, int reason, uint32_t thread_id);
    void (*sched_yield)(void *ctx, int reason, uint32_t thread_id);
    void (*make_ready)(void *ctx, uint32_t thread_id);
    void (*make_ready_fn)(void *ctx, uint32_t thread_id);
    void (*on_make_ready)(void *ctx, uint32_t thread_id);
    void (*sched_make_ready)(void *ctx, uint32_t thread_id);
    void *sched_ctx;
    void *context;
    void *user_ctx;
    void *user_data;
    void *ctx;
} ezhle_desc_t;

#define EZHLE_DESC_MIN_SIZE (offsetof(ezhle_desc_t, sched) + sizeof(const ezhle_sched_t *))

#define EZHLE_DESC_INIT(desc, system_, build_id_, bus_) do {            \
    memset((desc), 0, sizeof(*(desc)));                                 \
    (desc)->magic = EZHLE_ABI_MAGIC;                                    \
    (desc)->abi_version = EZHLE_ABI_VERSION;                            \
    (desc)->struct_size = sizeof(*(desc));                               \
    (desc)->system = (system_);                                         \
    (desc)->build_id = (build_id_);                                     \
    (desc)->bus = (bus_);                                               \
} while (0)

ezhle_core_t *ezhle_create(const ezhle_desc_t *desc);
void ezhle_destroy(ezhle_core_t *core);
void ezhle_reset(ezhle_core_t *core);
int ezhle_dispatch(ezhle_core_t *core, ezhle_regs_t *regs);
int ezhle_exited(const ezhle_core_t *core);
int32_t ezhle_exit_code(const ezhle_core_t *core);
const char *ezhle_last_error(const ezhle_core_t *core);
const char *ezhle_system(const ezhle_core_t *core);

#ifdef __cplusplus
}
#endif

#endif