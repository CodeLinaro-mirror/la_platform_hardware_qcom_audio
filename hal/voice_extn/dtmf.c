/* Copyright (c) 2018, The Linux Foundation. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of The Linux Foundation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.*/

#define LOG_TAG "dtmf"
/*#define LOG_NDEBUG 0*/
#define LOG_NDDEBUG 0

#include <errno.h>
#include <math.h>
#include <cutils/log.h>

#include "audio_hw.h"
#include "platform.h"
#include "platform_api.h"
#include "voice_extn.h"
#include <stdlib.h>
#include <cutils/str_parms.h>

#ifdef DYNAMIC_LOG_ENABLED
#include <log_xml_parser.h>
#define LOG_MASK HAL_MOD_FILE_DTMF
#include <log_utils.h>
#endif

#ifdef DTMF_ENABLED
int voice_extn_dtmf_generate_rx_tone(struct stream_out *out,
                                       uint32_t dtmf_low_freq,
                                       uint32_t dtmf_high_freq)
{
    struct audio_device *adev = out->dev;
    struct platform_data *my_data = (struct platform_data *)adev->platform;
    struct mixer_ctl *ctl = NULL;
    char *mixer_ctl_name = "DTMF_Generate Rx Low High Duration Gain";
    int ret = 0;
    //0xFFFF for duration is read as -1 (infinite) in int16
    long set_values[ ] = {0,
                          0,
                          0xFFFF,
                          out->rx_dtmf_tone_gain};

    set_values[0] = dtmf_low_freq;
    set_values[1] = dtmf_high_freq;
    ctl = mixer_get_ctl_by_name(adev->mixer, mixer_ctl_name);
    if (!ctl) {
        ALOGE("%s: Could not get ctl for mixer cmd - %s",
              __func__, mixer_ctl_name);
        return -EINVAL;
    }

    ALOGV("%s: Setting Rx DTMF: low:%d, high:%d gain:%d mixer ctrl:%s",
          __func__,
          dtmf_low_freq, dtmf_high_freq, out->rx_dtmf_tone_gain, mixer_ctl_name);
    mixer_ctl_set_array(ctl, set_values, ARRAY_SIZE(set_values));

    return ret;
}

int voice_extn_dtmf_set_rx_tone_gain(struct stream_out *out, int32_t gain)
{
    struct audio_device *adev = out->dev;
    struct platform_data *my_data = (struct platform_data *)adev->platform;
    int ret = 0;

    out->rx_dtmf_tone_gain = gain;
    return ret;
}

int voice_extn_dtmf_set_rx_tone_off(struct stream_out *out)
{
    struct audio_device *adev = out->dev;
    struct platform_data *my_data = (struct platform_data *)adev->platform;
    struct mixer_ctl *ctl = NULL;
    char *mixer_ctl_name = "DTMF_Generate Rx Low High Duration Gain";
    int ret = 0;
    //Duration set to 0 to disable tone
    long set_values[ ] = {0, 0, 0, 0};

    ctl = mixer_get_ctl_by_name(adev->mixer, mixer_ctl_name);
    if (!ctl) {
        ALOGE("%s: Could not get ctl for mixer cmd - %s",
              __func__, mixer_ctl_name);
        return -EINVAL;
    }

    ALOGV("%s: Setting Rx DTMF tone to disable: mixer ctrl:%s",
          __func__,
          mixer_ctl_name);
    mixer_ctl_set_array(ctl, set_values, ARRAY_SIZE(set_values));

    return ret;
}

int voice_extn_dtmf_set_rx_detection(struct stream_out *out,
                                   uint32_t session_id,
                                   bool enable)
{
    struct audio_device *adev = out->dev;
    struct platform_data *my_data = (struct platform_data *)adev->platform;
    struct mixer_ctl *ctl = NULL;
    struct listnode *node = NULL;
    struct audio_usecase *usecase = NULL;
    struct audio_adsp_event adsp_event_params = {AUDIO_STREAM_PP_EVENT,
                                                0,
                                                NULL};
    //adsp_hdlr only supports PCM_PLAYBACK flag for now
    struct adsp_hdlr_stream_cfg config = {0,
                                          0,
                                          PCM_PLAYBACK};
    char *mixer_ctl_name = "DTMF_Detect Rx VSID enable";
    int ret = 0;
    long set_values[ ] = {session_id, enable};

    ctl = mixer_get_ctl_by_name(adev->mixer, mixer_ctl_name);
    if (!ctl) {
        ALOGE("%s: Could not get ctl for mixer cmd - %s",
              __func__, mixer_ctl_name);
        return -EINVAL;
    }

    if (out->adsp_hdlr_stream_handle == NULL) {
        list_for_each(node, &adev->usecase_list) {
            usecase = node_to_item(node, struct audio_usecase, list);
            if (usecase->type == VOICE_CALL)
                config.pcm_device_id = platform_get_pcm_device_id(usecase->id, PCM_PLAYBACK);
        }
        ret =  audio_extn_adsp_hdlr_stream_open(&out->adsp_hdlr_stream_handle,
            &config);
        if (ret) {
            ALOGE("%s: Failed to open adsp_hdlr stream",
                  __func__);
            return ret;
        }
    }

    if (enable) {
        ret = audio_extn_adsp_hdlr_stream_set_param(out->adsp_hdlr_stream_handle,
            ADSP_HDLR_STREAM_CMD_REGISTER_EVENT,
            (void *)&adsp_event_params);
        if (ret) {
            ALOGE("%s: Failed to register adsp_hdlr event",
                  __func__);
            return ret;
        }
    } else {
        ret = audio_extn_adsp_hdlr_stream_set_param(out->adsp_hdlr_stream_handle,
            ADSP_HDLR_STREAM_CMD_DEREGISTER_EVENT,
            (void *)&adsp_event_params);
        if (ret) {
            ALOGE("%s: Failed to deregister adsp_hdlr event",
                  __func__);
            return ret;
        }
    }

    ALOGV("%s: Setting Rx DTMF detect to %d: mixer ctrl:%s",
          __func__,
          enable, mixer_ctl_name);
    mixer_ctl_set_array(ctl, set_values, ARRAY_SIZE(set_values));

    mixer_ctl_name = "DTMF_Detect Rx Callback VSID enable";
    ctl = mixer_get_ctl_by_name(adev->mixer, mixer_ctl_name);
    if (!ctl) {
        ALOGE("%s: Could not get ctl for mixer cmd - %s",
              __func__, mixer_ctl_name);
        return -EINVAL;
    }

    ALOGV("%s: Setting Rx DTMF detect cb evt to %d on session %d: mixer ctrl:%s",
          __func__,
          session_id, enable, mixer_ctl_name);
    mixer_ctl_set_array(ctl, set_values, ARRAY_SIZE(set_values));

    return ret;
}
#endif /*DTMF_ENABLED*/
