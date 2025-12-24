/*
 * Copyright (c) 2013-2021, The Linux Foundation. All rights reserved.
 * Not a contribution.
 *
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "voice"
/*#define LOG_NDEBUG 0*/
#define LOG_NDDEBUG 0

#include <errno.h>
#include <stdlib.h>
#include <math.h>
#include <log/log.h>
#include <cutils/str_parms.h>

#include "audio_hw.h"
#include "voice.h"
#include "voice_extn/voice_extn.h"
#include "platform.h"
#include "platform_api.h"
#include "audio_extn.h"

#ifdef DYNAMIC_LOG_ENABLED
#include <log_xml_parser.h>
#define LOG_MASK HAL_MOD_FILE_VOICE
#include <log_utils.h>
#endif

struct pcm_config pcm_config_voice_call = {
    .channels = 1,
    .rate = 8000,
    .period_size = 160,
    .period_count = 2,
    .format = PCM_FORMAT_S16_LE,
};

#ifdef PLATFORM_AUTO
#define VOICE_RX_VOLUME "Playback 36 Volume"
#define PLAYBACK_VOLUME_MAX 0x2000
#define PLAYBACK_VOLUME_DEFAULT (15.0)
#define CAPTURE_VOLUME_DEFAULT (15.0)
struct pcm *voice_loopback_tx = NULL;
struct pcm *voice_loopback_rx = NULL;
#endif
static struct voice_session *voice_get_session_from_use_case(struct audio_device *adev,
                              audio_usecase_t usecase_id)
{
    struct voice_session *session = NULL;
    int ret = 0;

    ret = voice_extn_get_session_from_use_case(adev, usecase_id, &session);
    if (ret == -ENOSYS) {
        session = &adev->voice.session[VOICE_SESS_IDX];
    }

    return session;
}

static bool voice_is_sidetone_device(snd_device_t out_device,
            char *mixer_path)
{
    bool is_sidetone_dev;

    switch (out_device) {
    case SND_DEVICE_OUT_VOICE_HANDSET:
        is_sidetone_dev = true;
        strlcpy(mixer_path, "sidetone-handset", MIXER_PATH_MAX_LENGTH);
        break;
    case SND_DEVICE_OUT_VOICE_HEADPHONES:
    case SND_DEVICE_OUT_VOICE_ANC_HEADSET:
    case SND_DEVICE_OUT_VOICE_ANC_FB_HEADSET:
        is_sidetone_dev = true;
        strlcpy(mixer_path, "sidetone-headphones", MIXER_PATH_MAX_LENGTH);
        break;
    case SND_DEVICE_OUT_VOICE_USB_HEADSET:
    case SND_DEVICE_OUT_USB_HEADSET:
        // USB does not use a QC mixer.
        mixer_path[0] = '\0';
        is_sidetone_dev = true;
        break;
    default:
        ALOGW("%s: %d is not a sidetone device", __func__, out_device);
        is_sidetone_dev = false;
        break;
    }

    return is_sidetone_dev;
}

void voice_set_sidetone(struct audio_device *adev,
        snd_device_t out_snd_device, bool enable)
{
    char mixer_path[MIXER_PATH_MAX_LENGTH];
    ALOGD("%s: %s, out_snd_device: %d\n",
          __func__, (enable ? "enable" : "disable"),
          out_snd_device);
    if (voice_is_sidetone_device(out_snd_device, mixer_path))
        platform_set_sidetone(adev, out_snd_device, enable, mixer_path);
    return;
}

static bool voice_is_aanc_device(snd_device_t out_device,
                                 char *mixer_path)
{
    bool is_aanc_dev;

    switch (out_device) {
        case SND_DEVICE_OUT_ANC_HANDSET:
            is_aanc_dev = true;
            strlcpy(mixer_path, "aanc-path", MIXER_PATH_MAX_LENGTH);
            break;
        default:
            is_aanc_dev = false;
            break;
    }

    return is_aanc_dev;
}

void voice_check_and_update_aanc_path(struct audio_device *adev,
                                      snd_device_t out_snd_device,
                                      bool enable)
{
    char mixer_path[MIXER_PATH_MAX_LENGTH];

    ALOGV("%s: %s, out_snd_device: %d\n",
          __func__, (enable ? "enable" : "disable"), out_snd_device);

    if (voice_is_aanc_device(out_snd_device, mixer_path))
        platform_update_aanc_path(adev, out_snd_device, enable, mixer_path);

    return;
}

int voice_stop_usecase(struct audio_device *adev, audio_usecase_t usecase_id)
{
    int ret = 0;
    struct audio_usecase *uc_info;
    struct voice_session *session = NULL;

    ALOGD("%s: enter usecase:%s", __func__, use_case_table[usecase_id]);

    session = (struct voice_session *)voice_get_session_from_use_case(adev, usecase_id);
    if (!session) {
        ALOGE("stop_call: couldn't find voice session");
        return -EINVAL;
    }

    uc_info = get_usecase_from_list(adev, usecase_id);
    if (uc_info == NULL) {
        ALOGE("%s: Could not find the usecase (%d) in the list",
              __func__, usecase_id);
        return -EINVAL;
    }

    /*  Unregister READY event before tearing down route/devices */
    if (uc_info->stream.out) {
        ALOGD("%s: Unregistering voice READY event", __func__);
        voice_extn_register_voice_ready_event(uc_info->stream.out, false);
    }

    session->state.current = CALL_INACTIVE;

    /* Disable sidetone only when no calls are active */
    if (!voice_is_call_state_active(adev))
        voice_set_sidetone(adev, uc_info->out_snd_device, false);

    /* Disable aanc only when no calls are active */
    if (!voice_is_call_state_active(adev))
        voice_check_and_update_aanc_path(adev, uc_info->out_snd_device, false);

    ret = platform_stop_voice_call(adev->platform, session->vsid);

    /* 1. Close the PCM devices */
    if (session->pcm_rx) {
        ret = pcm_close(session->pcm_rx);
        if (ret) {
           ALOGE("%s: pcm_close RX failed, rc = %d, errno = %d (%s)", __func__,
                  ret, errno, strerror(errno));
           ret = -errno;
        }
        session->pcm_rx = NULL;
    }
    if (session->pcm_tx) {
        ret = pcm_close(session->pcm_tx);
        if (ret) {
           ALOGE("%s: pcm_close TX failed, rc = %d, errno = %d (%s)", __func__,
                 ret, errno, strerror(errno));
           ret = -errno;
        }
        session->pcm_tx = NULL;
    }

#ifdef PLATFORM_AUTO
    if(voice_loopback_rx) {
        pcm_close(voice_loopback_rx);
        voice_loopback_rx = NULL;
    }
    if(voice_loopback_tx) {
        pcm_close(voice_loopback_tx);
        voice_loopback_tx = NULL;
    }
#endif
    /* 2. Get and set stream specific mixer controls */
    disable_audio_route(adev, uc_info);

    /* 3. Disable the rx and tx devices */
    disable_snd_device(adev, uc_info->out_snd_device);
    disable_snd_device(adev, uc_info->in_snd_device);

    adev->voice.in_ecall = 0;

    list_remove(&uc_info->list);
    free(uc_info);

    ALOGD("%s: exit: status(%d)", __func__, ret);
    return ret;
}

int voice_start_usecase(struct audio_device *adev, audio_usecase_t usecase_id)
{
    int ret = 0;
    struct audio_usecase *uc_info;
    int pcm_dev_rx_id, pcm_dev_tx_id;
#ifdef PLATFORM_AUTO
    int pcm_dev_loopback_rx_id, pcm_dev_loopback_tx_id;
#endif
    uint32_t sample_rate = 8000;
    struct voice_session *session = NULL;
    struct pcm_config voice_config = pcm_config_voice_call;

    ALOGD("%s: enter usecase:%s", __func__, use_case_table[usecase_id]);

    session = (struct voice_session *)voice_get_session_from_use_case(adev, usecase_id);
    if (!session) {
        ALOGE("start_call: couldn't find voice session");
        return -EINVAL;
    }

    uc_info = (struct audio_usecase *)calloc(1, sizeof(struct audio_usecase));
    if (!uc_info) {
        ALOGE("start_call: couldn't allocate mem for audio_usecase");
        return -ENOMEM;
    }

    uc_info->id = usecase_id;
    uc_info->type = VOICE_CALL;

 if( usecase_id == USECASE_VOICEMMODE1_CALL)
 {
    uc_info->stream.out = adev->voice_tx_output;
    uc_info->devices = adev->voice_tx_output->devices;
 }

 if( usecase_id == USECASE_VOICEMMODE2_CALL)
 {
     uc_info->stream.out = adev->voice2_tx_output;
     uc_info->devices = adev->voice2_tx_output->devices;
 }


    if (popcount(uc_info->devices) == 2) {
        ALOGE("%s: Invalid combo device(%#x) for voice call", __func__,
              uc_info->devices);
        ret = -EIO;
        goto error_start_voice;
    }

    uc_info->in_snd_device = SND_DEVICE_NONE;
    uc_info->out_snd_device = SND_DEVICE_NONE;
    adev->voice.use_device_mute = false;

    adev->voice.in_ecall = uc_info->stream.out->ecall;
    if (audio_is_bluetooth_sco_device(uc_info->devices) && !adev->bt_sco_on) {
        ALOGE("start_call: couldn't find BT SCO, SCO is not ready");
        adev->voice.in_call = false;
        ret = -EIO;
        goto error_start_voice;
    }

    list_add_tail(&adev->usecase_list, &uc_info->list);

    select_devices(adev, usecase_id);

#ifdef PLATFORM_AUTO
    pcm_dev_loopback_rx_id = VOICE_ASM_RX_TX;
    pcm_dev_loopback_tx_id = VOICE_ASM_RX_TX;
#endif
    pcm_dev_rx_id = platform_get_pcm_device_id(uc_info->id, PCM_PLAYBACK);
    pcm_dev_tx_id = platform_get_pcm_device_id(uc_info->id, PCM_CAPTURE);

    if (pcm_dev_rx_id < 0 || pcm_dev_tx_id < 0) {
        ALOGE("%s: Invalid PCM devices (rx: %d tx: %d) for the usecase(%d)",
              __func__, pcm_dev_rx_id, pcm_dev_tx_id, uc_info->id);
        ret = -EIO;
        goto error_start_voice;
    }
    ret = platform_get_sample_rate(adev->platform, &sample_rate);
    if (ret < 0) {
        ALOGE("platform_get_sample_rate error %d\n", ret);
    } else {
        voice_config.rate = sample_rate;
    }
    ALOGD("voice_config.rate %d\n", voice_config.rate);

    voice_set_mic_mute(adev, session->mic_mute, usecase_id);

    ALOGV("%s: Opening PCM capture device card_id(%d) device_id(%d)",
          __func__, adev->snd_card, pcm_dev_tx_id);
    session->pcm_tx = pcm_open(adev->snd_card,
                               pcm_dev_tx_id,
                               PCM_IN, &voice_config);
    if (session->pcm_tx && !pcm_is_ready(session->pcm_tx)) {
        ALOGE("%s: %s", __func__, pcm_get_error(session->pcm_tx));
        ret = -EIO;
        goto error_start_voice;
    }

    ALOGV("%s: Opening PCM playback device card_id(%d) device_id(%d)",
          __func__, adev->snd_card, pcm_dev_rx_id);
    session->pcm_rx = pcm_open(adev->snd_card,
                               pcm_dev_rx_id,
                               PCM_OUT, &voice_config);
    if (session->pcm_rx && !pcm_is_ready(session->pcm_rx)) {
        ALOGE("%s: %s", __func__, pcm_get_error(session->pcm_rx));
        ret = -EIO;
        goto error_start_voice;
    }

#ifdef PLATFORM_AUTO
    voice_loopback_rx = pcm_open(adev->snd_card,
                                 pcm_dev_loopback_rx_id,
                                 PCM_OUT, &voice_config);
    if (voice_loopback_rx < 0 || !pcm_is_ready(voice_loopback_rx)) {
        ALOGE("%s: Either could not open pcm_dev_loopback_rx_id %d or %s",
              __func__, pcm_dev_loopback_rx_id, pcm_get_error(voice_loopback_rx));
        ret = -EIO;
        goto error_start_voice;
    }

    voice_loopback_tx = pcm_open(adev->snd_card,
                                 pcm_dev_loopback_tx_id,
                                 PCM_IN, &voice_config);
    if (voice_loopback_tx < 0 || !pcm_is_ready(voice_loopback_tx)) {
         ALOGE("%s: Either could not open pcm_dev_loopback_tx_id %d or %s",
               __func__, pcm_dev_loopback_tx_id, pcm_get_error(voice_loopback_tx));
         ret = -EIO;
         goto error_start_voice;
    }
#endif

    if (uc_info->stream.out) {
        ret = voice_extn_register_voice_ready_event(uc_info->stream.out, true);
        if (ret) {
            ALOGE("%s: Failed to register voice READY event, ret=%d", __func__, ret);
            goto error_start_voice;
        }
    }

    if(adev->mic_break_enabled)
        platform_set_mic_break_det(adev->platform, true);

    ret = pcm_start(session->pcm_tx);
    if (ret) {
        ALOGE("%s: pcm_start TX failed rc = %d, errno = %d (%s)", __func__,
              ret, errno, strerror(errno));
        ret = -errno;
        goto error_start_voice;
    }
    ret = pcm_start(session->pcm_rx);
    if (ret) {
        ALOGE("%s: pcm_start RX failed rc = %d, errno = %d (%s)", __func__,
              ret, errno, strerror(errno));
        ret = -errno;
        goto error_start_voice;
    }
#ifdef PLATFORM_AUTO
    pcm_start(voice_loopback_tx);
    pcm_start(voice_loopback_rx);
#endif

    /* Enable aanc only when no calls are active */
    if (!voice_is_call_state_active(adev))
        voice_check_and_update_aanc_path(adev, uc_info->out_snd_device, true);

    /* Enable sidetone only when no calls are already active */
    if (!voice_is_call_state_active(adev))
        voice_set_sidetone(adev, uc_info->out_snd_device, true);

    voice_set_volume(adev, session->volume, session->vsid);

    ret = platform_start_voice_call(adev->platform, session->vsid);
    if (ret < 0) {
        ALOGE("%s: platform_start_voice_call error %d\n", __func__, ret);
        goto error_start_voice;
    }

    session->state.current = CALL_ACTIVE;
    goto done;

error_start_voice:
    voice_stop_usecase(adev, usecase_id);

done:
    ALOGD("%s: exit: status(%d)", __func__, ret);
    return ret;
}

bool voice_is_call_state_active(struct audio_device *adev)
{
    bool call_state = false;
    int ret = 0;

    ret = voice_extn_is_call_state_active(adev, &call_state);
    if (ret == -ENOSYS) {
        call_state = (adev->voice.session[VOICE_SESS_IDX].state.current == CALL_ACTIVE) ? true : false;
    }

    return call_state;
}

bool voice_is_call_usecase_active(struct audio_device *adev, audio_usecase_t usecase_id)
{
    bool call_state = false;
    int ret = 0;
    struct voice_session *session = NULL;

    session = (struct voice_session *)voice_get_session_from_use_case(adev, usecase_id);

    if (!session) {
        ALOGE("voice_is_call_usecase_active: couldn't find voice session");
        return call_state;
    }

    if(session->state.current != CALL_INACTIVE){
        call_state = true;
	}

    return call_state;
}

bool voice_is_in_call(const struct audio_device *adev)
{
    return adev->voice.in_call;
}

bool voice_is_in_ecall(const struct audio_device *adev)
{
    return adev->voice.in_ecall;
}

bool voice_is_in_call_rec_stream(const struct stream_in *in)
{
    bool in_call_rec = false;

    if (!in) {
       ALOGE("%s: input stream is NULL", __func__);
       return in_call_rec;
    }

    if (in->source == AUDIO_SOURCE_VOICE_DOWNLINK ||
        in->source == AUDIO_SOURCE_VOICE_UPLINK ||
        in->source == AUDIO_SOURCE_VOICE_CALL) {
            in_call_rec = true;
    }

    return in_call_rec;
}

uint32_t voice_get_active_session_id(struct audio_device *adev)
{
    int ret = 0;
    uint32_t session_id;

    ret = voice_extn_get_active_session_id(adev, &session_id);
    if (ret == -ENOSYS) {
        session_id = VOICE_VSID;
    }
    return session_id;
}

int voice_check_and_set_incall_rec_usecase(struct audio_device *adev,
                                           struct stream_in *in)
{
    int ret = 0;
    uint32_t session_id;
    int rec_mode = INCALL_REC_NONE;

    if (voice_is_call_state_active(adev)) {
        switch (in->source) {
        case AUDIO_SOURCE_VOICE_UPLINK:
            if (audio_extn_compr_cap_enabled() &&
                audio_extn_compr_cap_format_supported(in->config.format)) {
                in->usecase = USECASE_INCALL_REC_UPLINK_COMPRESS;
            } else
                in->usecase = USECASE_INCALL_REC_UPLINK;
            rec_mode = INCALL_REC_UPLINK;
            break;
        case AUDIO_SOURCE_VOICE_DOWNLINK:
            if (audio_extn_compr_cap_enabled() &&
                audio_extn_compr_cap_format_supported(in->config.format)) {
                in->usecase = USECASE_INCALL_REC_DOWNLINK_COMPRESS;
            } else
                in->usecase = USECASE_INCALL_REC_DOWNLINK;
            rec_mode = INCALL_REC_DOWNLINK;
            break;
        case AUDIO_SOURCE_VOICE_CALL:
            if (audio_extn_compr_cap_enabled() &&
                audio_extn_compr_cap_format_supported(in->config.format)) {
                in->usecase = USECASE_INCALL_REC_UPLINK_AND_DOWNLINK_COMPRESS;
            } else
                in->usecase = USECASE_INCALL_REC_UPLINK_AND_DOWNLINK;
            rec_mode = INCALL_REC_UPLINK_AND_DOWNLINK;
            break;
        default:
            ALOGV("%s: Source type %d doesnt match incall recording criteria",
                  __func__, in->source);
            return ret;
        }

        session_id = voice_get_active_session_id(adev);
        ret = platform_set_incall_recording_session_id(adev->platform,
                                                       session_id, rec_mode);
        ret = platform_set_incall_recording_session_channels(adev->platform,
                                                        in->config.channels);
        ALOGV("%s: Update usecase to %d",__func__, in->usecase);
    } else {
        /*
         * Reject the recording instances, where the recording is started
         * with In-call voice recording source types but voice call is not
         * active by the time input is started
         */
        if ((in->source == AUDIO_SOURCE_VOICE_UPLINK) ||
            (in->source == AUDIO_SOURCE_VOICE_DOWNLINK) ||
            (in->source == AUDIO_SOURCE_VOICE_CALL)) {
            ret = -EINVAL;
            ALOGE("%s: As voice call is not active, Incall rec usecase can't be \
                   selected for requested source:%d",__func__, in->source);
        }
        ALOGV("%s: voice call not active", __func__);
    }

    return ret;
}

int voice_check_and_stop_incall_rec_usecase(struct audio_device *adev,
                                            struct stream_in *in)
{
    int ret = 0;

    if (in->source == AUDIO_SOURCE_VOICE_UPLINK ||
        in->source == AUDIO_SOURCE_VOICE_DOWNLINK ||
        in->source == AUDIO_SOURCE_VOICE_CALL) {
        ret = platform_stop_incall_recording_usecase(adev->platform);
        ALOGV("%s: Stop In-call recording", __func__);
    }

    return ret;
}

snd_device_t voice_get_incall_rec_backend_device(struct stream_in *in)
{
   snd_device_t incall_record_device = {0};

    if (!in) {
       ALOGE("%s: input stream is NULL", __func__);
       return 0;
    }

   switch(in->source) {
    case AUDIO_SOURCE_VOICE_UPLINK:
        incall_record_device = SND_DEVICE_IN_INCALL_REC_TX;
        break;
    case AUDIO_SOURCE_VOICE_DOWNLINK:
         incall_record_device = SND_DEVICE_IN_INCALL_REC_RX;
         break;
    case AUDIO_SOURCE_VOICE_CALL:
         incall_record_device = SND_DEVICE_IN_INCALL_REC_RX_TX;
         break;
    default:
        ALOGI("Invalid source %d", in->source);
    }

   return incall_record_device;
}

snd_device_t voice_get_incall_rec_snd_device(snd_device_t in_snd_device)
{
    snd_device_t incall_record_device = in_snd_device;

    /*
     * For incall recording stream, AUDIO_COPP topology will be picked up
     * from the calibration data of the input sound device which is nothing
     * but the voice call's input device. But there are requirements to use
     * AUDIO_COPP_MONO topology even if the voice call's input device is
     * different. Hence override the input device with the one which uses
     * the AUDIO_COPP_MONO topology.
     */
    switch(in_snd_device) {
    case SND_DEVICE_IN_HANDSET_MIC:
    case SND_DEVICE_IN_VOICE_DMIC:
    case SND_DEVICE_IN_AANC_HANDSET_MIC:
        incall_record_device = SND_DEVICE_IN_HANDSET_MIC;
        break;
    case SND_DEVICE_IN_VOICE_SPEAKER_MIC:
    case SND_DEVICE_IN_VOICE_SPEAKER_DMIC:
    case SND_DEVICE_IN_VOICE_SPEAKER_DMIC_BROADSIDE:
    case SND_DEVICE_IN_VOICE_SPEAKER_QMIC:
        incall_record_device = SND_DEVICE_IN_VOICE_SPEAKER_MIC;
        break;
    default:
        incall_record_device = in_snd_device;
    }

    ALOGD("%s: in_snd_device(%d: %s) incall_record_device(%d: %s)", __func__,
          in_snd_device, platform_get_snd_device_name(in_snd_device),
          incall_record_device,  platform_get_snd_device_name(incall_record_device));

    return incall_record_device;
}

#ifdef PLATFORM_AUTO
/*Set mic volume to value.
*
* This interface is used for mic volume control, set mic volume as value(range 0 ~ 15).
*/
static int voice_set_mic_volume(struct audio_device *adev, float value)
{
    int volume, ret = 0;
    char mixer_ctl_name[128];
    struct mixer_ctl *ctl;
    int pcm_device_id = VOICE_ASM_RX_TX;

    if (value < 0.0) {
        ALOGW("%s: (%f) Under 0.0, assuming 0.0\n", __func__, value);
        value = 0.0;
    } else if (value > CAPTURE_VOLUME_DEFAULT) {
        value = CAPTURE_VOLUME_DEFAULT;
        ALOGW("%s: Volume brought within range (%f)\n", __func__, value);
    }

    value = value / CAPTURE_VOLUME_DEFAULT;
    memset(mixer_ctl_name, 0, sizeof(mixer_ctl_name));
    snprintf(mixer_ctl_name, sizeof(mixer_ctl_name),
            "Playback %d Volume", pcm_device_id);
    ctl = mixer_get_ctl_by_name(adev->mixer, mixer_ctl_name);
    if (!ctl) {
        ALOGE("%s: Could not get ctl for mixer cmd - %s",
                __func__, mixer_ctl_name);
        return -EINVAL;
    }
    volume = (int)(value * PLAYBACK_VOLUME_MAX);

    ALOGD("%s: Setting volume to %d (%s)\n", __func__, volume, mixer_ctl_name);
    if (mixer_ctl_set_value(ctl, 0, volume) < 0) {
        ALOGE("%s: Couldn't set Voice Call Volume: [%d]", __func__, volume);
        return -EINVAL;
    }

    return ret;
}

static float voice_get_mic_volume(struct audio_device *adev)
{
    int volume;
    char mixer_ctl_name[128];
    struct mixer_ctl *ctl;
    int pcm_device_id = VOICE_ASM_RX_TX;
    float value = 0.0;
    memset(mixer_ctl_name, 0, sizeof(mixer_ctl_name));
    snprintf(mixer_ctl_name, sizeof(mixer_ctl_name),
            "Playback %d Volume", pcm_device_id);
    ctl = mixer_get_ctl_by_name(adev->mixer, mixer_ctl_name);
    if (!ctl) {
        ALOGE("%s: Could not get ctl for mixer cmd - %s",
                __func__, mixer_ctl_name);
        return -EINVAL;
    }

    volume = mixer_ctl_get_value(ctl, 0);
    if ( volume < 0) {
        ALOGE("%s: Couldn't get Voice Call Volume: [%d]", __func__, volume);
        return -EINVAL;
    }
    ALOGD("%s: getting mic volume %d \n", __func__, volume);

    value = (volume / PLAYBACK_VOLUME_MAX) * CAPTURE_VOLUME_DEFAULT;
    if (value < 0.0) {
        ALOGW("%s: (%f) Under 0.0, assuming 0.0\n", __func__, value);
        value = 0.0;
    } else if (value > CAPTURE_VOLUME_DEFAULT) {
        value = CAPTURE_VOLUME_DEFAULT;
        ALOGW("%s: Volume brought within range (%f)\n", __func__, value);
    }
    return value;
}
#endif

int voice_set_mic_mute(struct audio_device *adev, bool state,
                       audio_usecase_t usecase_id)
{
    int err = 0;
    struct voice_session *session = NULL;
    uint32_t vsid = ALL_VSID;
    session = (struct voice_session *)voice_get_session_from_use_case(adev, usecase_id);
    if (session)
        session->mic_mute = state;
    if (session && ((usecase_id == USECASE_VOICEMMODE1_CALL) ||
        (usecase_id == USECASE_VOICEMMODE2_CALL)))
        vsid = session->vsid;
#ifdef PLATFORM_AUTO
    if (state == true) {
        adev->voice.mic_volume = voice_get_mic_volume(adev);
    }
    err = voice_set_mic_volume(adev, (state == true) ? 0.0 : adev->voice.mic_volume);
    ALOGD("%s: Setting mute state %d, err %d\n", __func__, state, err);
#else
    if (audio_extn_hfp_is_active(adev)) {
        err = hfp_set_mic_mute(adev, state);
    } else if (adev->mode == AUDIO_MODE_IN_CALL) {
       /* Use device mute if incall music delivery usecase is in progress */
        if (adev->voice.use_device_mute)
            err = platform_set_device_mute(adev->platform, state, "tx", vsid);
        else
            err = platform_set_mic_mute(adev->platform, state, vsid);
        ALOGV("%s: voice mute status=%d, use_device_mute flag=%d",
            __func__, state, adev->voice.use_device_mute);
    } else if (adev->mode == AUDIO_MODE_IN_COMMUNICATION) {
        err = voice_extn_compress_voip_set_mic_mute(adev, state);
    }
#endif
    return err;
}

bool voice_get_mic_mute(struct audio_device *adev, audio_usecase_t usecase_id)
{
    struct voice_session *session = NULL;
    bool mute_status = false;

    session = (struct voice_session *)voice_get_session_from_use_case(adev, usecase_id);
    if (session)
        mute_status = session->mic_mute;
    return mute_status;
}

/*
 * Following function is called when incall music uplink usecase is
 * created or destroyed while mic is muted. If incall music uplink
 * usecase is active, apply voice device mute to mute only voice Tx
 * path and not the mixed voice Tx + inncall-music path. Revert to
 * voice stream mute once incall music uplink usecase is inactive
 */
void voice_set_device_mute_flag(struct audio_device *adev, bool state)
{
    struct voice_session *session = NULL;
    uint32_t vsid;
    audio_usecase_t usecase_id = -1;

    vsid = voice_get_active_session_id(adev);
    usecase_id = voice_extn_get_usecase_for_session_id(vsid);
    session = (struct voice_session *)voice_get_session_from_use_case(adev, usecase_id);

    if ((session) && (session->mic_mute)) {
        if (state) {
            platform_set_device_mute(adev->platform, true, "tx", vsid);
            platform_set_mic_mute(adev->platform, false, vsid);
        } else {
            platform_set_mic_mute(adev->platform, true, vsid);
            platform_set_device_mute(adev->platform, false, "tx", vsid);
        }
    }
    adev->voice.use_device_mute = state;
}

int voice_set_volume(struct audio_device *adev, float volume, uint32_t vsid)
{
    int32_t vol, err = 0;

#ifdef PLATFORM_AUTO
    struct mixer_ctl *ctl;
    const char *mixer_ctl_name = VOICE_RX_VOLUME;
    if (volume < 0.0) {
        ALOGW("%s: (%f) Under 0.0, assuming 0.0\n", __func__, volume);
        volume = 0.0;
    } else {
        volume = ((volume > PLAYBACK_VOLUME_DEFAULT) ? 1.0 : (volume / PLAYBACK_VOLUME_DEFAULT));
        ALOGW("%s: Volume brought with in range (%f)\n", __func__, volume);
    }
    vol  = lrint((volume * PLAYBACK_VOLUME_MAX) + 0.5);
    ALOGD("%s: Setting Voice Cal volume to %d \n", __func__, vol);

    ctl = mixer_get_ctl_by_name(adev->mixer, mixer_ctl_name);
    if (!ctl) {
        ALOGE("%s: Could not get ctl for mixer cmd - %s",
                __func__, mixer_ctl_name);
        return -EINVAL;
    }
    if(mixer_ctl_set_value(ctl, 0, vol) < 0) {
        ALOGE("%s: Couldn't set Voice Call Volume: [%d]", __func__, vol);
        return -EINVAL;
    }
#else
    if (adev->mode == AUDIO_MODE_IN_CALL) {
        if (volume < 0.0) {
            volume = 0.0;
        } else if (volume > 1.0) {
            volume = 1.0;
        }

        vol = lrint(volume * 100.0);

        // Voice volume levels from android are mapped to driver volume levels as follows.
        // 0 -> 5, 20 -> 4, 40 ->3, 60 -> 2, 80 -> 1, 100 -> 0
        // So adjust the volume to get the correct volume index in driver
        vol = 100 - vol;

        err = platform_set_voice_volume(adev->platform, vol, vsid);
    }
    if (adev->mode == AUDIO_MODE_IN_COMMUNICATION)
        err = voice_extn_compress_voip_set_volume(adev, volume);
#endif
    return err;
}

int voice_get_volume(struct audio_device *adev, float *volume, uint32_t vsid)
{
    int32_t vol, err = 0;
    if (adev->mode == AUDIO_MODE_IN_CALL) {
        platform_get_voice_volume(adev->platform, &vol, vsid);

        vol = 100 - vol;
        *volume = (vol / 100.0);
        if (*volume < 0.0) {
            *volume = 0.0;
        } else if (*volume > 1.0) {
            *volume = 1.0;
        }
    }
    return err;
}

int voice_start_call(struct audio_device *adev, audio_usecase_t usecase_id)
{
    int ret = 0;
    struct voice_session *session = NULL;

    adev->voice.in_call = true;
    session = (struct voice_session *)voice_get_session_from_use_case(adev, usecase_id);
    if (session)
        voice_set_mic_mute(adev, session->mic_mute, usecase_id);

    ret = voice_extn_start_call(adev);
    if (ret == -ENOSYS) {
        ret = voice_start_usecase(adev, USECASE_VOICE_CALL);
    }

    return ret;
}

int voice_stop_call(struct audio_device *adev)
{
    int ret = 0;

    adev->voice.in_call = false;
    ret = voice_extn_stop_call(adev);
    if (ret == -ENOSYS) {
        ret = voice_stop_usecase(adev, USECASE_VOICE_CALL);
    }

    return ret;
}

void voice_get_parameters(struct audio_device *adev,
                          struct str_parms *query,
                          struct str_parms *reply)
{
    voice_extn_get_parameters(adev, query, reply);
}

int voice_set_parameters(struct audio_device *adev, struct str_parms *parms)
{
    char value[32];
    int ret = 0, err;
    char *kv_pairs = str_parms_to_str(parms);

    ALOGV_IF(kv_pairs != NULL, "%s: enter: %s", __func__, kv_pairs);

    ret = voice_extn_set_parameters(adev, parms);
    if (ret != 0) {
        if (ret == -ENOSYS)
            ret = 0;
        else
            goto done;
    }

    ret = voice_extn_compress_voip_set_parameters(adev, parms);
    if (ret != 0) {
        if (ret == -ENOSYS)
            ret = 0;
        else
            goto done;
    }

    err = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_TTY_MODE, value, sizeof(value));
    if (err >= 0) {
        int tty_mode;
        str_parms_del(parms, AUDIO_PARAMETER_KEY_TTY_MODE);
        if (strcmp(value, AUDIO_PARAMETER_VALUE_TTY_OFF) == 0)
            tty_mode = TTY_MODE_OFF;
        else if (strcmp(value, AUDIO_PARAMETER_VALUE_TTY_VCO) == 0)
            tty_mode = TTY_MODE_VCO;
        else if (strcmp(value, AUDIO_PARAMETER_VALUE_TTY_HCO) == 0)
            tty_mode = TTY_MODE_HCO;
        else if (strcmp(value, AUDIO_PARAMETER_VALUE_TTY_FULL) == 0)
            tty_mode = TTY_MODE_FULL;
        else {
            ret = -EINVAL;
            goto done;
        }

        if (tty_mode != adev->voice.tty_mode) {
            adev->voice.tty_mode = tty_mode;
            adev->acdb_settings = (adev->acdb_settings & TTY_MODE_CLEAR) | tty_mode;
            if (voice_is_call_state_active(adev))
               voice_update_devices_for_all_voice_usecases(adev);
        }
    }

    err = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_HAC,
                            value, sizeof(value));
    if (err >= 0) {
        bool hac = false;
        str_parms_del(parms, AUDIO_PARAMETER_KEY_HAC);
        if (strcmp(value, AUDIO_PARAMETER_VALUE_HAC_ON) == 0)
            hac = true;

        if (hac != adev->voice.hac) {
            adev->voice.hac = hac;
            if (voice_is_in_call(adev))
                voice_update_devices_for_all_voice_usecases(adev);
        }
    }

    err = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_INCALLMUSIC,
                            value, sizeof(value));
    if (err >= 0) {
        str_parms_del(parms, AUDIO_PARAMETER_KEY_INCALLMUSIC);
        if (strcmp(value, AUDIO_PARAMETER_VALUE_TRUE) == 0)
            platform_start_incall_music_usecase(adev->platform);
        else
            platform_stop_incall_music_usecase(adev->platform);
    }

done:
    ALOGV("%s: exit with code(%d)", __func__, ret);
    free(kv_pairs);
    return ret;
}

void voice_init(struct audio_device *adev)
{
    int i = 0;

    memset(&adev->voice, 0, sizeof(adev->voice));
    adev->voice.tty_mode = TTY_MODE_OFF;
    adev->voice.hac = false;
    adev->voice.in_call = false;
#ifdef PLATFORM_AUTO
    adev->voice.mic_volume = CAPTURE_VOLUME_DEFAULT;
#endif
    for (i = 0; i < MAX_VOICE_SESSIONS; i++) {
        adev->voice.session[i].pcm_rx = NULL;
        adev->voice.session[i].pcm_tx = NULL;
        adev->voice.session[i].state.current = CALL_INACTIVE;
        adev->voice.session[i].state.new = CALL_INACTIVE;
        adev->voice.session[i].vsid = VOICE_VSID;
        adev->voice.session[i].volume = 1.0f;
        adev->voice.session[i].mic_mute = false;
    }

    voice_extn_init(adev);
}

void voice_update_devices_for_all_voice_usecases(struct audio_device *adev)
{
    struct listnode *node;
    struct audio_usecase *usecase;

    list_for_each(node, &adev->usecase_list) {
        usecase = node_to_item(node, struct audio_usecase, list);
        if (usecase->type == VOICE_CALL) {
            ALOGV("%s: updating device for usecase:%s", __func__,
                  use_case_table[usecase->id]);
            if(usecase->id == USECASE_VOICEMMODE1_CALL)
            	usecase->stream.out = adev->voice_tx_output;
            if(usecase->id == USECASE_VOICEMMODE2_CALL)
            	usecase->stream.out = adev->voice2_tx_output;
            select_devices(adev, usecase->id);
        }
    }
}


