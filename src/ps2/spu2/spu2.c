#include "ps2/spu2/spu2.h"

#include "ezhle/ezhle.h"

int ps2_spu2_render_frame(const ps2_host_audio_t *host_audio)
{
    (void)host_audio;
    /* SPU2 audio via host core is not implemented: the shared ABI (ezhle-2)
     * exposes no host audio output callback in ezhle_bus_t and no audio
     * surface in ezhle_desc_t, so rendered samples cannot be delivered to
     * the host core. See docs/PS2-AUD-001-BLOCKER.md. */
    return EZHLE_ERR_UNIMPL;
}