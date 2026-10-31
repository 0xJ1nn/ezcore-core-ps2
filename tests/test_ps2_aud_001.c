#include "ezhle/ezhle.h"
#include "ezhle/ezhle_ps2.h"

#include <stdio.h>

static void push_samples(void *ctx, const int16_t *samples, size_t frames,
                         uint32_t sample_rate)
{
    (void)ctx;
    (void)samples;
    (void)frames;
    (void)sample_rate;
}

int main(void)
{
    ps2_host_audio_t host;
    host.push_samples = push_samples;
    host.ctx = NULL;

    /* SPU2 audio via host core is unimplemented: no host audio hook exists in
     * the shared ABI (ezhle-2), so rendering returns EZHLE_ERR_UNIMPL. */
    if (ps2_spu2_render_frame(&host) != EZHLE_ERR_UNIMPL) {
        fprintf(stderr, "FAIL: expected EZHLE_ERR_UNIMPL\n");
        return 1;
    }
    if (ps2_spu2_render_frame(NULL) != EZHLE_ERR_UNIMPL) {
        fprintf(stderr, "FAIL: NULL host must also be UNIMPL\n");
        return 1;
    }

    printf("PS2-AUD-001: SPU2 audio via host core is UNIMPL (missing host audio hook)\n");
    return 0;
}