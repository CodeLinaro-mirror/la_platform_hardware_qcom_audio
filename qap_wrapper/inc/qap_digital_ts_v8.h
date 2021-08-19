 /*
  * Copyright (c) 2017,2020 The Linux Foundation. All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without
  * modification, are permitted provided that the following conditions are
  * met:
  *  * Redistributions of source code must retain the above copyright
  *    notice, this list of conditions and the following disclaimer.
  *  * Redistributions in binary form must reproduce the above
  *    copyright notice, this list of conditions and the following
  *    disclaimer in the documentation and/or other materials provided
  *    with the distribution.
  *  * Neither the name of The Linux Foundation nor the names of its
  *    contributors may be used to endorse or promote products derived
  *    from this software without specific prior written permission.
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


#ifndef DTS_M8_H
#define DTS_M8_H

#include "qap_api.h"
#include "qap_defs.h"
#include "qap_module.h"

#if __cplusplus
extern "C" {
#endif

/* Add New Params for dts */
typedef enum {
    DTS_SESSION_CFG_AD_MIXING_ENABLED,                /* AD mixing on/off */
    DTS_SESSION_CFG_AD_MIXING_USER_PREF,              /* AD mixing user preference */
    DTS_SESSION_CFG_SYSSOUND_MIXING_ENABLED,          /* System sound mixing on/off */
} dts_m8_session_param_t;

/* TODO: Add params related dap for DTS */
typedef enum {
    DTS_DAP_CFG_CAL_BOOST,            /* DAP Calibration Boost */
} dts_ms8_dap_param_t;

/* Dts m8 metadata struct */
typedef struct dts_m8_gain_metadata {
    bool is_metadata_enable;
    bool is_primary_audio_gain_valid;
    bool is_secondary_pann_gain_valid;
    bool is_post_mix_gain_valid;

    uint32_t primary_audio_channel_mask;
    uint32_t primary_audio_gain[DTS_MAX_CHANNELS]; //Q15
    uint32_t secondary_pann_gain[DTS_MAX_CHANNELS]; //Q15
    uint32_t post_mix_gain; //Q20
} dts_m8_gain_metadata_t;

/* TODO: Define metadata struct as per DTS */
#if __cplusplus
}  // extern "C"
#endif

#endif //DTS_M8_H
