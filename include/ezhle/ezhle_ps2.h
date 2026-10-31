#ifndef EZHLE_EZHLE_PS2_H
#define EZHLE_EZHLE_PS2_H

#include "ezhle/ezhle.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* PS2 SPU2 audio via host core.
 *
 * The host core supplies an audio output hook so the HLE can push rendered
 * samples without dereferencing guest pointers. In ABI ezhle-2
 * (include/ezhle/ezhle.h) ezhle_bus_t has NO audio output callback and
 * ezhle_desc_t has NO audio surface, so this hook does not exist yet.
 * See docs/PS2-AUD-001-BLOCKER.md.
 */
typedef struct ps2_host_audio {
    void (*push_samples)(void *ctx, const int16_t *samples, size_t frames,
                         uint32_t sample_rate);
    void *ctx;
} ps2_host_audio_t;

/* Returns EZHLE_ERR_UNIMPL until the host audio hook lands in the shared ABI. */
int ps2_spu2_render_frame(const ps2_host_audio_t *host_audio);

#ifdef __cplusplus
}
#endif

#endif