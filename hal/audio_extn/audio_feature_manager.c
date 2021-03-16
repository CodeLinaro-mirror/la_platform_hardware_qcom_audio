/*
 * Copyright (c) 2019, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of The Linux Foundation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "audio_feature_manager"

#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <cutils/properties.h>
#include <log/log.h>
#include <unistd.h>
#include "audio_hw.h"
#include "audio_extn.h"
#include "voice_extn.h"
#include "audio_feature_manager.h"

extern AHalValues* confValues;

void audio_feature_manager_init()
{
    ALOGV("%s: Enter", __func__);
    audio_extn_ahal_config_helper_init(
                isRunningWithVendorEnhancedFramework());
    confValues = audio_extn_get_feature_values();
    audio_extn_feature_init();
    voice_extn_feature_init();
}

bool audio_feature_manager_is_feature_enabled(audio_ext_feature feature)
{
    ALOGV("%s: Enter", __func__);

#ifdef AHAL_EXT_ENABLED
    if (!audio_extn_is_config_from_remote())
        confValues = audio_extn_get_feature_values();
#endif /* AHAL_EXT_ENABLED */

    if (!confValues)
        return false;

    switch (feature) {
        case SND_MONITOR:
            return confValues->snd_monitor_enabled;
        case AFE_PROXY:
            return confValues->afe_proxy_enabled;
        case USE_DEEP_BUFFER_AS_PRIMARY_OUTPUT:
            return confValues->use_deep_buffer_as_primary_output;
        case RECEIVER_AIDED_STEREO:
            return confValues->receiver_aided_stereo;
        case KPI_OPTIMIZE:
            return confValues->kpi_optimize_enabled;
        case FLUENCE:
            return confValues->fluence_enabled;
        case ANC_HEADSET:
            return confValues->anc_headset_enabled;
        case DSM_FEEDBACK:
            return confValues->dsm_feedback_enabled;
        case VBAT:
            return confValues->vbat_enabled;
        case DYNAMIC_ECNS:
            return confValues->dynamic_ecns_enabled;
        case SPKR_PROT:
            return confValues->spkr_prot_enabled;
        default:
            return false;
    }
}
