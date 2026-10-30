/* PS2 audio output determinism — clean-room, no firmware/BIOS/ROM. */

#include "ezhle/ezhle_ps2.h"

#include <stdint.h>
#include <string.h>

#define FRAME_COUNT 2048

typedef struct { int32_t l; int32_t r; } aud_sample_t;

/* Deterministic PRNG (xorshift32) — no global state. */
static uint32_t rng_next(uint32_t *s)
{
    uint32_t x = *s;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *s = x;
    return x;
}

static void gen_input(aud_sample_t *buf, size_t n, uint32_t seed)
{
    uint32_t s = seed ? seed : 0xA5A5A5A5u;
    for (size_t i = 0; i < n; ++i) {
        uint32_t a = rng_next(&s);
        uint32_t b = rng_next(&s);
        buf[i].l = (int32_t)(a & 0x0000FFFFu) - 32768;
        buf[i].r = (int32_t)(b & 0x0000FFFFu) - 32768;
    }
}

static int32_t clamp32(int64_t v)
{
    if (v > 2147483647LL) return 2147483647;
    if (v < -2147483648LL) return -2147483648LL;
    return (int32_t)v;
}

/* Deterministic mix: gain + clamp, 1:1 frame copy. Returns 0 on success. */
static int mix_frames(const aud_sample_t *in, size_t n,
                      int32_t *out_lr, size_t out_cap)
{
    if (n * 2 > out_cap) return 1;
    for (size_t i = 0; i < n; ++i) {
        out_lr[2 * i]     = clamp32((int64_t)in[i].l * 2);
        out_lr[2 * i + 1] = clamp32((int64_t)in[i].r * 2);
    }
    return 0;
}

/* Deterministic linear resampler (fixed ratio 48000->44100) using fixed point. */
static size_t resample(const int32_t *in_lr, size_t in_frames,
                       int32_t *out_lr, size_t out_cap)
{
    /* ratio in fixed point: out_frames ~= in_frames * 441/480 */
    size_t out_frames = (in_frames * 441u) / 480u;
    if (out_frames * 2 > out_cap) return 0;
    uint32_t pos = 0; /* 16.16 fixed point into input */
    const uint32_t step = (480u << 16) / 441u;
    for (size_t o = 0; o < out_frames; ++o) {
        size_t i = pos >> 16;
        uint32_t frac = pos & 0xFFFFu;
        if (i + 1 >= in_frames) {
            out_lr[2 * o]     = in_lr[2 * i];
            out_lr[2 * o + 1] = in_lr[2 * i + 1];
        } else {
            int64_t l0 = in_lr[2 * i];
            int64_t l1 = in_lr[2 * (i + 1)];
            int64_t r0 = in_lr[2 * i + 1];
            int64_t r1 = in_lr[2 * (i + 1) + 1];
            out_lr[2 * o]     = (int32_t)(l0 + ((l1 - l0) * (int64_t)frac) / 65536);
            out_lr[2 * o + 1] = (int32_t)(r0 + ((r1 - r0) * (int64_t)frac) / 65536);
        }
        pos += step;
    }
    return out_frames;
}

static int run_once(uint32_t seed, int32_t *final_buf, size_t final_cap,
                   size_t *final_n)
{
    aud_sample_t in[FRAME_COUNT];
    int32_t mixed[FRAME_COUNT * 2];
    size_t mixed_n = 0;

    gen_input(in, FRAME_COUNT, seed);
    if (mix_frames(in, FRAME_COUNT, mixed, FRAME_COUNT * 2) != 0) return 1;
    mixed_n = FRAME_COUNT;

    size_t out_frames = resample(mixed, mixed_n, final_buf, final_cap);
    if (out_frames == 0) return 2;
    *final_n = out_frames * 2;
    return 0;
}

int ezhle_ps2_audio_output_determinism_selftest(void)
{
    int32_t a[FRAME_COUNT * 2];
    int32_t b[FRAME_COUNT * 2];
    size_t na = 0, nb = 0;

    if (run_once(0x9E3779B9u, a, FRAME_COUNT * 2, &na) != 0) return 10;
    if (run_once(0x9E3779B9u, b, FRAME_COUNT * 2, &nb) != 0) return 11;

    if (na != nb) return 20;
    if (memcmp(a, b, na * sizeof(int32_t)) != 0) return 21;

    /* Different seed must not crash and must produce a valid buffer. */
    int32_t c[FRAME_COUNT * 2];
    size_t nc = 0;
    if (run_once(0x12345678u, c, FRAME_COUNT * 2, &nc) != 0) return 30;
    if (nc == 0 || nc != na) return 31;

    return 0;
}
