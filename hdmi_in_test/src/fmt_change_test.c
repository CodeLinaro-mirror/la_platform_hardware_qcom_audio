/*
 * Copyright (c) 2016-2019, The Linux Foundation. All rights reserved.
 * Not a Contribution.
 *
 * Copyright (C) 2015 The Android Open Source Project *
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

/* Test app to capture event updates from kernel */
/*#define LOG_NDEBUG 0*/
#include <getopt.h>
#include <fcntl.h>
#include <linux/netlink.h>
#include <pthread.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <utils/Log.h>
#include <math.h>
#include <signal.h>
#include <errno.h>
#include "qahw_api.h"
#include "qahw_defs.h"

/* add local define to prevent compilation errors on other platforms */
#ifndef AUDIO_DEVICE_IN_HDMI_ARC
#define AUDIO_DEVICE_IN_HDMI_ARC (AUDIO_DEVICE_BIT_IN | 0x8000000)
#endif

#define MAX_RECORD_SESSIONS 4

static int sock_event_fd = -1;

void *context = NULL;
FILE * log_file = NULL;
volatile bool stop_test = false;
volatile bool stop_record[MAX_RECORD_SESSIONS] = {false};
volatile bool record_active[MAX_RECORD_SESSIONS] = {false};

int num_sessions = 1;

#define HDMI_SYS_PATH "/sys/devices/platform/soc/78b7000.i2c/i2c-3/3-0064/"
const char hdmi_in_audio_sys_path[] = HDMI_SYS_PATH "link_on0";
const char hdmi_in_power_on_sys_path[] = HDMI_SYS_PATH "power_on";
const char hdmi_in_audio_path_sys_path[] = HDMI_SYS_PATH "audio_path";
const char hdmi_in_arc_enable_sys_path[] = HDMI_SYS_PATH "arc_enable";

const char hdmi_in_audio_state_sys_path[] = HDMI_SYS_PATH "audio_state";
const char hdmi_in_audio_format_sys_path[] = HDMI_SYS_PATH "audio_format";
const char hdmi_in_audio_sample_rate_sys_path[] = HDMI_SYS_PATH "audio_rate";
const char hdmi_in_audio_layout_sys_path[] = HDMI_SYS_PATH "audio_layout";
const char hdmi_in_audio_ch_count_sys_path[] = HDMI_SYS_PATH "audio_ch_count";
const char hdmi_in_audio_ch_alloc_sys_path[] = HDMI_SYS_PATH "audio_ch_alloc";
const char hdmi_in_audio_preemph_sys_path[] = HDMI_SYS_PATH "audio_preemph";

#define SPDIF_SYS_PATH "/sys/devices/platform/soc/soc:qcom,msm-dai-q6-spdif-pri-tx/"
const char spdif_in_audio_state_sys_path[] = SPDIF_SYS_PATH "audio_state";
const char spdif_in_audio_format_sys_path[] = SPDIF_SYS_PATH "audio_format";
const char spdif_in_audio_sample_rate_sys_path[] = SPDIF_SYS_PATH "audio_rate";
const char spdif_in_audio_preemph_sys_path[] = SPDIF_SYS_PATH "audio_preemph";

#define SPDIF_ARC_SYS_PATH "/sys/devices/platform/soc/soc:qcom,msm-dai-q6-spdif-sec-tx/"
const char spdif_arc_in_audio_state_sys_path[] = SPDIF_ARC_SYS_PATH "audio_state";
const char spdif_arc_in_audio_format_sys_path[] = SPDIF_ARC_SYS_PATH "audio_format";
const char spdif_arc_in_audio_sample_rate_sys_path[] = SPDIF_ARC_SYS_PATH "audio_rate";
const char spdif_arc_in_audio_preemph_sys_path[] = SPDIF_ARC_SYS_PATH "audio_preemph";

#define TRANSCODE_LOOPBACK_SOURCE_PORT_ID 0x4C00
#define TRANSCODE_LOOPBACK_SINK_PORT_ID 0x4D00

#define ID_RIFF 0x46464952
#define ID_WAVE 0x45564157
#define ID_FMT  0x20746d66
#define ID_DATA 0x61746164

#define FORMAT_PCM 1

struct wav_header {
    uint32_t riff_id;
    uint32_t riff_sz;
    uint32_t riff_fmt;
    uint32_t fmt_id;
    uint32_t fmt_sz;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;       /* sample_rate * num_channels * bps / 8 */
    uint16_t block_align;     /* num_channels * bps / 8 */
    uint16_t bits_per_sample;
    uint32_t data_id;
    uint32_t data_sz;
};

struct test_data {
    /* following only used from first session */
    pthread_t poll_event_th;
    pthread_attr_t poll_event_attr;
    double record_length;

    /* following used separate in each session */
    int session_idx;
    qahw_module_handle_t *qahw_mod_handle;
    audio_io_handle_t handle;
    audio_devices_t input_device;
    int rec_cnt;
    pthread_t record_th;

    int bit_width;
    audio_input_flags_t flags;
    audio_config_t config;
    audio_source_t source;

    int spdif_audio_state;
    int spdif_audio_mode;
    int spdif_sample_rate;
    int spdif_preemph;
    int spdif_num_channels;

    int hdmi_power_on;
    int hdmi_audio_path;
    int hdmi_arc_enable;

    int hdmi_audio_state;
    int hdmi_audio_mode;
    int hdmi_audio_layout;
    int hdmi_sample_rate;
    int hdmi_preemph;
    int hdmi_num_channels;
    int hdmi_audio_ch_count;
    int hdmi_audio_ch_alloc;

    int spdif_arc_audio_state;
    int spdif_arc_audio_mode;
    int spdif_arc_sample_rate;
    int spdif_arc_preemph;
    int spdif_arc_num_channels;

    audio_devices_t new_input_device;

    audio_devices_t act_input_device; /* HDMI might use I2S and SPDIF */

    int act_audio_state;    /* audio active */
    int act_audio_mode;     /* 0=LPCM, 1=Compr */
    int act_sample_rate;    /* transmission sample rate */
    int act_preemph;        /* pcm signal has applied preemphase */
    int act_num_channels;   /* transmission channels */

    /* loopback only uses first session */
    int is_loopback;
    audio_devices_t sink_device;
    struct {
        struct audio_port_config source_config;
        struct audio_port_config sink_config;
        audio_patch_handle_t patch_handle;
        audio_format_t act_codec_format;
        audio_format_t new_codec_format;
        int ch_mask;
        float gain;
    } loopback;
};

struct test_data tdata[MAX_RECORD_SESSIONS];

int set_metadata_av_window_mat(qahw_module_handle_t *hw_module,
                               audio_patch_handle_t handle);

uint32_t check_audio_format(uint32_t audio_format)
{
    if ((audio_format == AUDIO_FORMAT_AC3) ||
        (audio_format == AUDIO_FORMAT_E_AC3) ||
        (audio_format == AUDIO_FORMAT_DOLBY_TRUEHD))
        return audio_format;

    fprintf(log_file, "unsupported audio_format: 0x%0x, using AC3\n",
        audio_format);
    return AUDIO_FORMAT_AC3;
}

void stop_signal_handler(int signal)
{
   stop_test = true;
}

int fmt_update_cb(qahw_stream_callback_event_t event, void *param, void *cookie)
{
    uint32_t *payload = param;
    int i;

    if(payload == NULL) {
        fprintf(log_file, "Invalid callback handle\n");
        fprintf(stderr, "Invalid callback handle\n");
        return 0;
    }

    switch (event) {
    case QAHW_STREAM_CBK_EVENT_ADSP:
        if ((payload[0] == QAHW_STREAM_IEC_61937_FMT_UPDATE_EVENT) &&
            (payload[1] == sizeof(int))) {
            fprintf(log_file, "received IEC_61937 update format: 0x%x \n", payload[2]);
            tdata[0].loopback.new_codec_format = check_audio_format(payload[2]);
        } else {
            fprintf(log_file, "received unexpected event\n");
            fprintf(log_file, "event_type %d\n", payload[0]);
            fprintf(log_file, "param_length %d\n", payload[1]);
            for (i=2; i*sizeof(uint32_t)<=payload[1]; i++)
                fprintf(log_file, "param[%d] = 0x%x\n", i, payload[i]);
        }
        break;
    default:
        break;
    }
    return 0;
}

int set_event_callback(qahw_module_handle_t *hw_module,
                       audio_patch_handle_t handle)
{
    qahw_loopback_param_payload payload;
    int ret = 0;

    fprintf(log_file, "Set callback using qahw_loopback_set_param_data\n");

    payload.stream_callback_params.cb = fmt_update_cb;
    payload.stream_callback_params.cookie = (void*)handle;

    ret = qahw_loopback_set_param_data(hw_module, handle,
        QAHW_PARAM_LOOPBACK_SET_CALLBACK, &payload);
    if (ret < 0) {
        fprintf(log_file, "qahw_loopback_set_param_data cb failed with err %d\n", ret);
        goto done;
    }

done:
    return ret;
}

void set_device_chmap_from_ca_byte(struct test_data *td, uint32_t ch_alloc)
{
    /*
     * this mapping is according to CEA-861-E Table 28
     * ch_alloc 0x00 can be used for 2ch and default 8ch
     * channel ID 47 marks unused channels
     */

    switch (ch_alloc) {
        case (0x00): /* L/R/LFE/C/Ls/Rs/Lb/Rb */
            qahw_set_parameters(td->qahw_mod_handle, "capture_device_chmap=8, 1, 2, 6, 3, 4, 5, 8, 9");
            break;
        case (0x01): /* L/R/LFE/-/-/-/-/- */
            qahw_set_parameters(td->qahw_mod_handle, "capture_device_chmap=8, 1, 2, 6,47,47,47,47,47");
            break;
        case (0x02): /* L/R/-/C/-/-/-/- */
            qahw_set_parameters(td->qahw_mod_handle, "capture_device_chmap=8, 1, 2,47, 3,47,47,47,47");
            break;
        case (0x03): /* L/R/LFE/C/-/-/-/- */
            qahw_set_parameters(td->qahw_mod_handle, "capture_device_chmap=8, 1, 2, 6, 3,47,47,47,47");
            break;
        case (0x04): /* L/R/-/-/Cs/-/-/- */
            qahw_set_parameters(td->qahw_mod_handle, "capture_device_chmap=8, 1, 2,47,47, 7,47,47,47");
            break;
        case (0x05): /* L/R/LFE/-/Cs/-/-/- */
            qahw_set_parameters(td->qahw_mod_handle, "capture_device_chmap=8, 1, 2, 6,47, 7,47,47,47");
            break;
        case (0x06): /* L/R/-/C/Cs/-/-/- */
            qahw_set_parameters(td->qahw_mod_handle, "capture_device_chmap=8, 1, 2,47, 3, 7,47,47,47");
            break;
        case (0x07): /* L/R/LFE/C/Cs/-/-/- */
            qahw_set_parameters(td->qahw_mod_handle, "capture_device_chmap=8, 1, 2, 6, 3, 7,47,47,47");
            break;
        case (0x08): /* L/R/-/-/Ls/Rs/-/- */
            qahw_set_parameters(td->qahw_mod_handle, "capture_device_chmap=8, 1, 2,47,47, 4, 5,47,47");
            break;
        case (0x09): /* L/R/LFE/-/Ls/Rs/-/- */
            qahw_set_parameters(td->qahw_mod_handle, "capture_device_chmap=8, 1, 2, 6,47, 4, 5,47,47");
            break;
        case (0x0A): /* L/R/-/C/Ls/Rs/-/- */
            qahw_set_parameters(td->qahw_mod_handle, "capture_device_chmap=8, 1, 2,47, 3, 4, 5,47,47");
            break;
        case (0x0B): /* L/R/LFE/C/Ls/Rs/-/- */
            qahw_set_parameters(td->qahw_mod_handle, "capture_device_chmap=8, 1, 2, 6, 3, 4, 5,47,47");
            break;
        case (0x0C): /* L/R/-/-/Ls/Rs/Cs/- */
            qahw_set_parameters(td->qahw_mod_handle, "capture_device_chmap=8, 1, 2,47,47, 4, 5, 7,47");
            break;
        case (0x0D): /* L/R/LFE/-/Ls/Rs/Cs/- */
            qahw_set_parameters(td->qahw_mod_handle, "capture_device_chmap=8, 1, 2, 6,47, 4, 5, 7,47");
            break;
        case (0x0E): /* L/R/-/C/Ls/Rs/Cs/- */
            qahw_set_parameters(td->qahw_mod_handle, "capture_device_chmap=8, 1, 2,47, 3, 4, 5, 7,47");
            break;
        case (0x0F): /* L/R/LFE/C/Ls/Rs/Cs/- */
            qahw_set_parameters(td->qahw_mod_handle, "capture_device_chmap=8, 1, 2, 6, 3, 4, 5, 7,47");
            break;
        case (0x10): /* L/R/-/-/Ls/Rs/Lb/Rb */
            qahw_set_parameters(td->qahw_mod_handle, "capture_device_chmap=8, 1, 2,47,47, 4, 5, 8, 9");
            break;
        case (0x11): /* L/R/LFE/-/Ls/Rs/Lb/Rb */
            qahw_set_parameters(td->qahw_mod_handle, "capture_device_chmap=8, 1, 2, 6,47, 4, 5, 8, 9");
            break;
        case (0x12): /* L/R/-/C/Ls/Rs/Lb/Rb */
            qahw_set_parameters(td->qahw_mod_handle, "capture_device_chmap=8, 1, 2,47, 3, 4, 5, 8, 9");
            break;
        case (0x13): /* L/R/LFE/C/Ls/Rs/Lb/Rb */
            qahw_set_parameters(td->qahw_mod_handle, "capture_device_chmap=8, 1, 2, 6, 3, 4, 5, 8, 9");
            break;
        case (0x14): /* L/R/-/-/-/-/Flc/Frc */
            qahw_set_parameters(td->qahw_mod_handle, "capture_device_chmap=8, 1, 2,47,47,47,47,13,14");
            break;
        case (0x15): /* L/R/LFE/-/-/-/Flc/Frc */
            qahw_set_parameters(td->qahw_mod_handle, "capture_device_chmap=8, 1, 2, 6,47,47,47,13,14");
            break;
        case (0x16): /* L/R/-/C/-/-/Flc/Frc */
            qahw_set_parameters(td->qahw_mod_handle, "capture_device_chmap=8, 1, 2,47, 3,47,47,13,14");
            break;
        case (0x17): /* L/R/LFE/C/-/-/Flc/Frc */
            qahw_set_parameters(td->qahw_mod_handle, "capture_device_chmap=8, 1, 2, 6, 3,47,47,13,14");
            break;
        case (0x18): /* L/R/-/-/Cs/-/Flc/Frc */
            qahw_set_parameters(td->qahw_mod_handle, "capture_device_chmap=8, 1, 2,47,47, 7,47,13,14");
            break;
        case (0x19): /* L/R/LFE/-/Cs/-/Flc/Frc */
            qahw_set_parameters(td->qahw_mod_handle, "capture_device_chmap=8, 1, 2, 6,47, 7,47,13,14");
            break;
        case (0x1A): /* L/R/-/C/Cs/-/Flc/Frc */
            qahw_set_parameters(td->qahw_mod_handle, "capture_device_chmap=8, 1, 2,47, 3, 7,47,13,14");
            break;
        case (0x1B): /* L/R/LFE/C/Cs/-/Flc/Frc */
            qahw_set_parameters(td->qahw_mod_handle, "capture_device_chmap=8, 1, 2, 6, 3, 7,47,13,14");
            break;
        case (0x1C): /* L/R/-/-/Ls/Rs/Flc/Frc */
            qahw_set_parameters(td->qahw_mod_handle, "capture_device_chmap=8, 1, 2,47,47, 4, 5,13,14");
            break;
        case (0x1D): /* L/R/LFE/-/Ls/Rs/Flc/Frc */
            qahw_set_parameters(td->qahw_mod_handle, "capture_device_chmap=8, 1, 2, 6,47, 4, 5,13,14");
            break;
        case (0x1E): /* L/R/-/C/Ls/Rs/Flc/Frc */
            qahw_set_parameters(td->qahw_mod_handle, "capture_device_chmap=8, 1, 2,47, 3, 4, 5,13,14");
            break;
        case (0x1F): /* L/R/LFE/C/Ls/Rs/Flc/Frc */
            qahw_set_parameters(td->qahw_mod_handle, "capture_device_chmap=8, 1, 2, 6, 3, 4, 5,13,14");
            break;
        case (0x20): /* L/R/-/C/Ls/Rs/Cvh/- */
            qahw_set_parameters(td->qahw_mod_handle, "capture_device_chmap=8, 1, 2,47, 3, 4, 5,11,47");
            break;
        case (0x21): /* L/R/LFE/C/Ls/Rs/Cvh/- */
            qahw_set_parameters(td->qahw_mod_handle, "capture_device_chmap=8, 1, 2, 6, 3, 4, 5,11,47");
            break;
        case (0x22): /* L/R/-/C/Ls/Rs/-/Tc */
            qahw_set_parameters(td->qahw_mod_handle, "capture_device_chmap=8, 1, 2,47, 3, 4, 5,47,22");
            break;
        case (0x23): /* L/R/LFE/C/Ls/Rs/-/Tc */
            qahw_set_parameters(td->qahw_mod_handle, "capture_device_chmap=8, 1, 2, 6, 3, 4, 5,47,22");
            break;
        case (0x24): /* L/R/-/-/Ls/Rs/Lvh/Rvh */
            qahw_set_parameters(td->qahw_mod_handle, "capture_device_chmap=8, 1, 2,47,47, 4, 5,20,21");
            break;
        case (0x25): /* L/R/LFE/-/Ls/Rs/Lvh/Rvh */
            qahw_set_parameters(td->qahw_mod_handle, "capture_device_chmap=8, 1, 2, 6,47, 4, 5,20,21");
            break;
        case (0x26): /* L/R/-/-/Ls/Rs/Lw/Rw */
            qahw_set_parameters(td->qahw_mod_handle, "capture_device_chmap=8, 1, 2,47,47, 4, 5,31,32");
            break;
        case (0x27): /* L/R/LFE/-/Ls/Rs/Lw/Rw */
            qahw_set_parameters(td->qahw_mod_handle, "capture_device_chmap=8, 1, 2, 6,47, 4, 5,31,32");
            break;
        case (0x28): /* L/R/-/C/Ls/Rs/Cs/Tc */
            qahw_set_parameters(td->qahw_mod_handle, "capture_device_chmap=8, 1, 2,47, 3, 4, 5, 7,22");
            break;
        case (0x29): /* L/R/LFE/C/Ls/Rs/Cs/Tc */
            qahw_set_parameters(td->qahw_mod_handle, "capture_device_chmap=8, 1, 2, 6, 3, 4, 5, 7,22");
            break;
        case (0x2A): /* L/R/-/C/Ls/Rs/Cs/Chv */
            qahw_set_parameters(td->qahw_mod_handle, "capture_device_chmap=8, 1, 2,47, 3, 4, 5, 7,11");
            break;
        case (0x2B): /* L/R/LFE/C/Ls/Rs/Cs/Chv */
            qahw_set_parameters(td->qahw_mod_handle, "capture_device_chmap=8, 1, 2, 6, 3, 4, 5, 7,11");
            break;
        case (0x2C): /* L/R/-/C/Ls/Rs/Cvh/Tc */
            qahw_set_parameters(td->qahw_mod_handle, "capture_device_chmap=8, 1, 2,47, 3, 4, 5,11,22");
            break;
        case (0x2D): /* L/R/LFE/C/Ls/Rs/Cvh/Tc */
            qahw_set_parameters(td->qahw_mod_handle, "capture_device_chmap=8, 1, 2, 6, 3, 4, 5,11,22");
            break;
        case (0x2E): /* L/R/-/C/Ls/Rs/Lvh/Rvh */
            qahw_set_parameters(td->qahw_mod_handle, "capture_device_chmap=8, 1, 2,47, 3, 4, 5,20,21");
            break;
        case (0x2F): /* L/R/LFE/C/Ls/Rs/Lvh/Rvh */
            qahw_set_parameters(td->qahw_mod_handle, "capture_device_chmap=8, 1, 2, 6, 3, 4, 5,20,21");
            break;
        case (0x30): /* L/R/-/C/Ls/Rs/Lw/Rw */
            qahw_set_parameters(td->qahw_mod_handle, "capture_device_chmap=8, 1, 2,47, 3, 4, 5,31,32");
            break;
        case (0x31): /* L/R/LFE/C/Ls/Rs/Lw/Rw */
            qahw_set_parameters(td->qahw_mod_handle, "capture_device_chmap=8, 1, 2, 6, 3, 4, 5,31,32");
            break;
        default:
            qahw_set_parameters(td->qahw_mod_handle, "capture_device_chmap=8, 1, 2, 6, 3, 4, 5, 8, 9");
            fprintf(log_file, "invalid channel allocation 0x%02x\n", td->hdmi_audio_ch_alloc);
            break;
    }
}

void start_loopback(struct test_data *td)
{
    int rc = 0;

    fprintf(log_file,"\nCreating audio patch\n");

    switch (td->act_num_channels) {
    case 2:
        td->config.channel_mask = AUDIO_CHANNEL_OUT_STEREO; /* use out define */
        set_device_chmap_from_ca_byte(td, 0x00);
        break;
    case 8:
        td->config.channel_mask = AUDIO_CHANNEL_OUT_7POINT1;
        if (td->is_loopback && !td->act_audio_mode)
            set_device_chmap_from_ca_byte(td, td->hdmi_audio_ch_alloc);
        else
            set_device_chmap_from_ca_byte(td, 0x00);
        break;
    default:
        fprintf(log_file,
            "ERROR :::: channel count %d not supported\n",
            td->act_num_channels);
        pthread_exit(0);
    }

    td->loopback.source_config.id = TRANSCODE_LOOPBACK_SOURCE_PORT_ID;
    td->loopback.source_config.role = AUDIO_PORT_ROLE_SOURCE;
    td->loopback.source_config.type = AUDIO_PORT_TYPE_DEVICE;
    td->loopback.source_config.config_mask = (AUDIO_PORT_CONFIG_ALL ^ AUDIO_PORT_CONFIG_GAIN);
    td->loopback.source_config.sample_rate = td->config.sample_rate;
    td->loopback.source_config.channel_mask = td->config.channel_mask;
    td->loopback.source_config.ext.device.hw_module = AUDIO_MODULE_HANDLE_NONE;
    td->loopback.source_config.ext.device.type = td->act_input_device;

    if (td->act_audio_mode)
        td->loopback.source_config.format = td->loopback.new_codec_format;
    else
        td->loopback.source_config.format = td->config.format;

    td->loopback.act_codec_format = td->loopback.new_codec_format;

    fprintf(log_file,"source config id %d, rate %d, ch_mask %d, format %d, dev %d\n",
        td->loopback.source_config.id,
        td->loopback.source_config.sample_rate,
        td->loopback.source_config.channel_mask,
        td->loopback.source_config.format,
        td->loopback.source_config.ext.device.type);

    td->loopback.sink_config.id = TRANSCODE_LOOPBACK_SINK_PORT_ID;
    td->loopback.sink_config.role = AUDIO_PORT_ROLE_SINK;
    td->loopback.sink_config.type = AUDIO_PORT_TYPE_DEVICE;
    td->loopback.sink_config.config_mask = (AUDIO_PORT_CONFIG_ALL ^ AUDIO_PORT_CONFIG_GAIN);
    td->loopback.sink_config.sample_rate = 48000;
    td->loopback.sink_config.channel_mask = td->loopback.ch_mask;
    td->loopback.sink_config.ext.device.hw_module = AUDIO_MODULE_HANDLE_NONE;
    td->loopback.sink_config.ext.device.type = td->sink_device;

    if (td->bit_width == 32)
        td->loopback.sink_config.format = AUDIO_FORMAT_PCM_8_24_BIT;
    else if (td->bit_width == 24)
        td->loopback.sink_config.format = AUDIO_FORMAT_PCM_24_BIT_PACKED;
    else
        td->loopback.sink_config.format = AUDIO_FORMAT_PCM_16_BIT;

    qahw_source_port_config_t source_port_config;
    qahw_sink_port_config_t sink_port_config;

    source_port_config.source_config = &td->loopback.source_config;
    sink_port_config.sink_config = &td->loopback.sink_config;

    source_port_config.flags = td->flags;
    sink_port_config.flags = td->flags;

    source_port_config.num_sources = 1;
    sink_port_config.num_sinks = 1;

    fprintf(log_file,"sink config id %d, rate %d, ch_mask %d, format %d, dev %d\n",
        td->loopback.sink_config.id,
        td->loopback.sink_config.sample_rate,
        td->loopback.sink_config.channel_mask,
        td->loopback.sink_config.format,
        td->loopback.sink_config.ext.device.type);

    rc = qahw_create_audio_patch_v2(td->qahw_mod_handle,
                        &source_port_config,
                        &sink_port_config,
                        &td->loopback.patch_handle);
    fprintf(log_file,"\nCreate patch returned %d\n",rc);
    if(!rc) {
        /* callback for MEDIA_FMT events */
        set_event_callback(td->qahw_mod_handle, td->loopback.patch_handle);

        struct audio_port_config sink_gain_config;
        /* Convert loopback gain to millibels */
        int loopback_gain_in_millibels = 2000 * log10(td->loopback.gain);
        sink_gain_config.gain.index = 0;
        sink_gain_config.gain.mode = AUDIO_GAIN_MODE_JOINT;
        sink_gain_config.gain.channel_mask = 1;
        sink_gain_config.gain.values[0] = loopback_gain_in_millibels;
        sink_gain_config.id = td->loopback.sink_config.id;
        sink_gain_config.role = td->loopback.sink_config.role;
        sink_gain_config.type = td->loopback.sink_config.type;
        sink_gain_config.config_mask = AUDIO_PORT_CONFIG_GAIN;

        if (td->loopback.new_codec_format == AUDIO_FORMAT_MAT) {
            set_metadata_av_window_mat(td->qahw_mod_handle, td->loopback.patch_handle);
        }

        (void)qahw_set_audio_port_config(td->qahw_mod_handle,
                    &sink_gain_config);
    } else {
        /* start again with default codec */
        td->loopback.new_codec_format = AUDIO_FORMAT_AC3;
        return;
    }

    while (true && !stop_record[td->session_idx]) {
        usleep(1000);
    }

    fprintf(log_file,"\nStopping current loopback session\n");
    if(td->loopback.patch_handle != AUDIO_PATCH_HANDLE_NONE)
        qahw_release_audio_patch(td->qahw_mod_handle,
                                 td->loopback.patch_handle);
    td->loopback.patch_handle = AUDIO_PATCH_HANDLE_NONE;
}

void *start_input(void *thread_param) {
    int rc = 0, ret = 0, count = 0;
    ssize_t bytes_read = -1;
    char file_name[256] = "/data/rec";
    int data_sz = 0, name_len = strlen(file_name);
    qahw_in_buffer_t in_buf;
    struct test_data *td = (struct test_data *)thread_param;

    if (num_sessions > 1) {
        snprintf(file_name, sizeof(file_name), "/data/rec_s%d_",
            td->session_idx);
        name_len = strlen(file_name);
    }

    qahw_module_handle_t *qahw_mod_handle = td->qahw_mod_handle;

    /* convert/check params before use */
    td->config.sample_rate = td->act_sample_rate;

    if (td->act_audio_mode) {
        td->config.format = AUDIO_FORMAT_IEC61937;
        td->flags = QAHW_INPUT_FLAG_COMPRESS | QAHW_INPUT_FLAG_PASSTHROUGH;
    } else {
        if (td->bit_width == 32)
            td->config.format = AUDIO_FORMAT_PCM_8_24_BIT;
        else if (td->bit_width == 24)
            td->config.format = AUDIO_FORMAT_PCM_24_BIT_PACKED;
        else
            td->config.format = AUDIO_FORMAT_PCM_16_BIT;
        td->flags = AUDIO_INPUT_FLAG_FAST | QAHW_INPUT_FLAG_COMPRESS;
    }

    td->config.frame_count = 0;

    if (td->is_loopback) {
        start_loopback(td);
        return NULL;
    }

    switch (td->act_num_channels) {
    case 2:
        td->config.channel_mask = AUDIO_CHANNEL_IN_STEREO;
        break;
    case 8:
        td->config.channel_mask = AUDIO_CHANNEL_INDEX_MASK_8;
        break;
    default:
        fprintf(log_file,
            "ERROR :::: channel count %d not supported\n",
            td->act_num_channels);
        pthread_exit(0);
    }
    set_device_chmap_from_ca_byte(td, 0x00);

    /* Open audio input stream */
    qahw_stream_handle_t* in_handle = NULL;

    rc = qahw_open_input_stream(qahw_mod_handle, td->handle,
        td->act_input_device, &td->config, &in_handle, td->flags,
        "input_stream", td->source);
    if (rc) {
        fprintf(log_file,
            "ERROR :::: Could not open input stream, handle(%d)\n",
            td->handle);
        fprintf(log_file,
            "rc=%d, in_handle=%p\n", rc, in_handle);
        pthread_exit(0);
    }

    /* Get buffer size to get upper bound on data to read from the HAL */
    size_t buffer_size = qahw_in_get_buffer_size(in_handle);
    char *buffer = (char *) calloc(1, buffer_size);
    size_t written_size;
    if (buffer == NULL) {
        fprintf(log_file, "calloc failed!!, handle(%d)\n", td->handle);
        pthread_exit(0);
    }

    fprintf(log_file, " input opened, buffer  %p, size %zu, handle(%d)\n", buffer,
        buffer_size, td->handle);

    /* set profile for the recording session */
    if (td->act_preemph == 1)
        qahw_in_set_parameters(in_handle, "audio_stream_profile=record_deemph");
    else
        qahw_in_set_parameters(in_handle, "audio_stream_profile=record_unprocessed");

    if (audio_is_linear_pcm(td->config.format))
        snprintf(file_name + name_len, sizeof(file_name) - name_len, "%d.wav",
            td->rec_cnt);
    else
        snprintf(file_name + name_len, sizeof(file_name) - name_len, "%d.raw",
            td->rec_cnt);

    td->rec_cnt++;

    FILE *fd = fopen(file_name, "w");
    if (fd == NULL) {
        fprintf(log_file, "File open failed\n");
        free(buffer);
        pthread_exit(0);
    }

    int bps = 16;

    switch (td->config.format) {
    case AUDIO_FORMAT_PCM_24_BIT_PACKED:
        bps = 24;
        break;
    case AUDIO_FORMAT_PCM_8_24_BIT:
    case AUDIO_FORMAT_PCM_32_BIT:
        bps = 32;
        break;
    case AUDIO_FORMAT_PCM_16_BIT:
    default:
        bps = 16;
    }

    struct wav_header hdr;
    hdr.riff_id = ID_RIFF;
    hdr.riff_sz = 0;
    hdr.riff_fmt = ID_WAVE;
    hdr.fmt_id = ID_FMT;
    hdr.fmt_sz = 16;
    hdr.audio_format = FORMAT_PCM;
    hdr.num_channels = td->act_num_channels;
    hdr.sample_rate = td->config.sample_rate;
    hdr.byte_rate = hdr.sample_rate * hdr.num_channels * (bps / 8);
    hdr.block_align = hdr.num_channels * (bps / 8);
    hdr.bits_per_sample = bps;
    hdr.data_id = ID_DATA;
    hdr.data_sz = 0;
    if (audio_is_linear_pcm(td->config.format))
        fwrite(&hdr, 1, sizeof(hdr), fd);

    memset(&in_buf, 0, sizeof(qahw_in_buffer_t));
    while (true && !stop_record[td->session_idx]) {
        in_buf.buffer = buffer;
        in_buf.bytes = buffer_size;
        bytes_read = qahw_in_read(in_handle, &in_buf);

        written_size = fwrite(in_buf.buffer, 1, bytes_read, fd);
        if (written_size < bytes_read) {
            printf("Error in fwrite(%d)=%s\n", ferror(fd),
                strerror(ferror(fd)));
            break;
        }
        data_sz += bytes_read;
    }

    if (audio_is_linear_pcm(td->config.format)) {
        /* update lengths in header */
        hdr.data_sz = data_sz;
        hdr.riff_sz = data_sz + 44 - 8;
        fseek(fd, 0, SEEK_SET);
        fwrite(&hdr, 1, sizeof(hdr), fd);
    }
    free(buffer);
    fclose(fd);
    fd = NULL;

    fprintf(log_file, " closing input, handle(%d), written %d bytes", td->handle, data_sz);

    /* Close input stream and device. */
    rc = qahw_in_standby(in_handle);
    if (rc) {
        fprintf(log_file, "in standby failed %d, handle(%d)\n", rc,
            td->handle);
    }

    rc = qahw_close_input_stream(in_handle);
    if (rc) {
        fprintf(log_file, "could not close input stream %d, handle(%d)\n", rc,
            td->handle);
    }

    fprintf(log_file,
        "\n\n The audio recording has been saved to %s.\n"
        "The audio data has the  following characteristics:\n Sample rate: %i\n Format: %d\n "
        "Num channels: %i, handle(%d)\n\n", file_name,
        td->config.sample_rate, td->config.format, td->act_num_channels,
        td->handle);

    return NULL;
}

void start_rec_thread(int session_idx)
{
    int ret = 0;

    if (stop_test)
        return;

    stop_record[session_idx] = false;
    record_active[session_idx] = true;

    fprintf(log_file, "\n Create session %d record thread \n", session_idx);
    ret = pthread_create(&tdata[session_idx].record_th, NULL, start_input, (void *)&tdata[session_idx]);
    if (ret) {
        fprintf(log_file, " Failed to create session %d record thread\n", session_idx);
        exit(1);
   }
}

void stop_rec_thread(int session_idx)
{
    if (record_active[session_idx]) {
        record_active[session_idx] = false;
        stop_record[session_idx] = true;
        fprintf(log_file, "\n Stop session %d record thread \n", session_idx);
        pthread_join(tdata[session_idx].record_th, NULL);
    }
}


void read_data_from_fd(const char* path, int *value)
{
    int fd = -1;
    char buf[16];
    int ret;

    fd = open(path, O_RDONLY, 0);
    if (fd < 0) {
        ALOGE("Unable open fd for file %s", path);
        return;
    }

    ret = read(fd, buf, 15);
    if (ret < 0) {
        ALOGE("File %s Data is empty", path);
        close(fd);
        return;
    }

    buf[ret] = '\0';
    *value = atoi(buf);
    close(fd);
}

void get_input_status(void)
{
    struct test_data *td = &tdata[0];
    int i;

    for (i=0; i<num_sessions; i++) {
        switch (td->input_device) {
        case AUDIO_DEVICE_IN_SPDIF:
            read_data_from_fd(spdif_in_audio_state_sys_path, &td->spdif_audio_state);
            read_data_from_fd(spdif_in_audio_format_sys_path, &td->spdif_audio_mode);
            read_data_from_fd(spdif_in_audio_sample_rate_sys_path, &td->spdif_sample_rate);
            read_data_from_fd(spdif_in_audio_preemph_sys_path, &td->spdif_preemph);
            td->spdif_num_channels = 2;
            td->new_input_device = AUDIO_DEVICE_IN_SPDIF;

            fprintf(log_file, "ses%d: spdif audio_state: %d, audio_format: %d, sample_rate: %d, num_channels: %d, preemph: %d\n",
                td->session_idx, td->spdif_audio_state, td->spdif_audio_mode,
                td->spdif_sample_rate, td->spdif_num_channels, td->spdif_preemph);
            if ((td->spdif_audio_mode) && (td->spdif_preemph)) {
                td->spdif_preemph = 0;
                fprintf(log_file, "ses%d: ignore wrong preemph in compressed mode\n",
                    td->session_idx);
            }
            break;
        case AUDIO_DEVICE_IN_HDMI:
            read_data_from_fd(hdmi_in_power_on_sys_path, &td->hdmi_power_on);
            read_data_from_fd(hdmi_in_audio_path_sys_path, &td->hdmi_audio_path);
            read_data_from_fd(hdmi_in_arc_enable_sys_path, &td->hdmi_arc_enable);

            read_data_from_fd(hdmi_in_audio_state_sys_path, &td->hdmi_audio_state);
            read_data_from_fd(hdmi_in_audio_format_sys_path, &td->hdmi_audio_mode);
            read_data_from_fd(hdmi_in_audio_sample_rate_sys_path, &td->hdmi_sample_rate);
            read_data_from_fd(hdmi_in_audio_layout_sys_path, &td->hdmi_audio_layout);
            read_data_from_fd(hdmi_in_audio_preemph_sys_path, &td->hdmi_preemph);

            if (td->hdmi_audio_layout)
                td->hdmi_num_channels = 8;
            else
                td->hdmi_num_channels = 2;

            /* read ch_count, ch_alloc to use in PCM mode */
            read_data_from_fd(hdmi_in_audio_ch_count_sys_path, &td->hdmi_audio_ch_count);
            read_data_from_fd(hdmi_in_audio_ch_alloc_sys_path, &td->hdmi_audio_ch_alloc);

            read_data_from_fd(spdif_arc_in_audio_state_sys_path, &td->spdif_arc_audio_state);
            read_data_from_fd(spdif_arc_in_audio_format_sys_path, &td->spdif_arc_audio_mode);
            read_data_from_fd(spdif_arc_in_audio_sample_rate_sys_path, &td->spdif_arc_sample_rate);
            read_data_from_fd(spdif_arc_in_audio_preemph_sys_path, &td->spdif_arc_preemph);
            td->spdif_arc_num_channels = 2;

            if (td->hdmi_arc_enable ||
                (td->hdmi_audio_state && (td->hdmi_audio_layout == 0) && td->hdmi_audio_mode)) {
                td->new_input_device = AUDIO_DEVICE_IN_HDMI_ARC;
                fprintf(log_file, "ses%d: hdmi audio interface SPDIF_ARC\n", td->session_idx);
            } else {
                td->new_input_device = AUDIO_DEVICE_IN_HDMI;
                fprintf(log_file, "ses%d: hdmi audio interface MI2S\n", td->session_idx);
            }

            fprintf(log_file, "ses%d: hdmi audio_state: %d, audio_format: %d, sample_rate: %d, num_channels: %d, preemph: %d\n",
                td->session_idx, td->hdmi_audio_state, td->hdmi_audio_mode,
                td->hdmi_sample_rate, td->hdmi_num_channels, td->hdmi_preemph);

            if (!td->hdmi_audio_mode)
                fprintf(log_file, "ses%d: hdmi pcm ch_count: %d, ch_alloc: %d\n",
                    td->session_idx, td->hdmi_audio_ch_count, td->hdmi_audio_ch_alloc);

            fprintf(log_file, "ses%d: arc  audio_state: %d, audio_format: %d, sample_rate: %d, num_channels: %d, preemph: %d\n",
                td->session_idx, td->spdif_arc_audio_state, td->spdif_arc_audio_mode,
                td->spdif_arc_sample_rate, td->spdif_arc_num_channels, td->spdif_arc_preemph);

            if ((td->hdmi_audio_mode) && (td->hdmi_preemph)) {
                td->hdmi_preemph = 0;
                fprintf(log_file, "ses%d: ignore wrong preemph in compressed mode\n",
                    td->session_idx);
            }
            if ((td->spdif_arc_audio_mode) && (td->spdif_arc_preemph)) {
                td->spdif_arc_preemph = 0;
                fprintf(log_file, "ses%d: ignore wrong preemph in compressed mode\n",
                    td->session_idx);
            }
            break;
        }
        td++;
    }
}

void input_restart_check(void)
{
    struct test_data *td = &tdata[0];
    int i;

    get_input_status();

    for (i=0; i<num_sessions; i++) {
        switch (td->input_device) {
        case AUDIO_DEVICE_IN_SPDIF:
            if ((td->act_input_device != td->new_input_device) ||
                (td->spdif_audio_state == 2) ||
                (td->spdif_preemph != td->act_preemph) ||
                (td->is_loopback && td->loopback.act_codec_format != td->loopback.new_codec_format)) {
                fprintf(log_file, "ses%d: old       audio_state: %d, audio_format: %d, rate: %d, channels: %d, preemph: %d\n",
                    td->session_idx, td->act_audio_state, td->act_audio_mode,
                    td->act_sample_rate, td->act_num_channels, td->act_preemph);
                fprintf(log_file, "ses%d: new spdif audio_state: %d, audio_format: %d, rate: %d, channels: %d, preemph: %d\n",
                    td->session_idx, td->spdif_audio_state, td->spdif_audio_mode,
                    td->spdif_sample_rate, td->spdif_num_channels, td->spdif_preemph);

                stop_rec_thread(td->session_idx);

                td->act_input_device = AUDIO_DEVICE_IN_SPDIF;
                td->act_audio_state = 1;
                td->act_audio_mode = td->spdif_audio_mode;
                td->act_sample_rate = td->spdif_sample_rate;
                td->act_num_channels = td->spdif_num_channels;
                td->act_preemph = td->spdif_preemph;

                start_rec_thread(td->session_idx);
            }
            break;
        case AUDIO_DEVICE_IN_HDMI:
            if (td->act_input_device != td->new_input_device) {
                stop_rec_thread(td->session_idx);

                if (td->new_input_device == AUDIO_DEVICE_IN_HDMI) {
                    fprintf(log_file, "ses%d: old      audio_state: %d, audio_format: %d, rate: %d, channels: %d, preemph: %d\n",
                        td->session_idx, td->act_audio_state, td->act_audio_mode,
                        td->act_sample_rate, td->act_num_channels, td->act_preemph);
                    fprintf(log_file, "ses%d: new hdmi audio_state: %d, audio_format: %d, rate: %d, channels: %d, preemph: %d\n",
                        td->session_idx, td->hdmi_audio_state, td->hdmi_audio_mode,
                        td->hdmi_sample_rate, td->hdmi_num_channels, td->hdmi_preemph);

                    td->act_input_device = AUDIO_DEVICE_IN_HDMI;
                    td->act_audio_state = td->hdmi_audio_state;
                    td->act_audio_mode = td->hdmi_audio_mode;
                    td->act_sample_rate = td->hdmi_sample_rate;
                    td->act_num_channels = td->hdmi_num_channels;
                    td->act_preemph = td->hdmi_preemph;

                    if (td->hdmi_audio_state)
                        start_rec_thread(td->session_idx);
                } else {
                    td->act_input_device = AUDIO_DEVICE_IN_HDMI_ARC;
                    if (td->hdmi_arc_enable) {
                        fprintf(log_file, "ses%d: old     audio_state: %d, audio_format: %d, rate: %d, channels: %d, preemph: %d\n",
                            td->session_idx, td->act_audio_state, td->act_audio_mode,
                            td->act_sample_rate, td->act_num_channels, td->act_preemph);
                        fprintf(log_file, "ses%d: new arc audio_state: %d, audio_format: %d, rate: %d, channels: %d, preemph: %d\n",
                            td->session_idx, td->spdif_arc_audio_state, td->spdif_arc_audio_mode,
                            td->spdif_arc_sample_rate, td->spdif_arc_num_channels, td->spdif_arc_preemph);

                        td->act_audio_state = 1;
                        td->act_audio_mode = td->spdif_arc_audio_mode;
                        td->act_sample_rate = td->spdif_arc_sample_rate;
                        td->act_num_channels = td->spdif_arc_num_channels;
                        td->act_preemph = td->spdif_arc_preemph;
                    } else {
                        fprintf(log_file, "ses%d: old      audio_state: %d, audio_format: %d, rate: %d, channels: %d, preemph: %d\n",
                            td->session_idx, td->act_audio_state, td->act_audio_mode,
                            td->act_sample_rate, td->act_num_channels, td->act_preemph);
                        fprintf(log_file, "ses%d: new arc (from hdmi) audio_state: %d, audio_format: %d, rate: %d, channels: %d, preemph: %d\n",
                            td->session_idx, td->hdmi_audio_state, td->hdmi_audio_mode,
                            td->hdmi_sample_rate, td->hdmi_num_channels, td->hdmi_preemph);

                        td->act_audio_state = 1;
                        td->act_audio_mode = td->hdmi_audio_mode;
                        td->act_sample_rate = td->hdmi_sample_rate;
                        td->act_num_channels = td->hdmi_num_channels;
                        td->act_preemph = td->hdmi_preemph;
                    }
                    start_rec_thread(td->session_idx);
                }
            } else { /* check for change on same audio device */
                if (td->new_input_device == AUDIO_DEVICE_IN_HDMI) {
                    if ((td->act_audio_state != td->hdmi_audio_state) ||
                        (td->act_audio_mode != td->hdmi_audio_mode) ||
                        (td->act_sample_rate != td->hdmi_sample_rate) ||
                        (td->act_num_channels != td->hdmi_num_channels) ||
                        (td->act_preemph != td->hdmi_preemph) ||
                        (td->is_loopback && td->loopback.act_codec_format != td->loopback.new_codec_format)) {

                        fprintf(log_file, "ses%d: old      audio_state: %d, audio_format: %d, rate: %d, channels: %d, preemph: %d\n",
                            td->session_idx, td->act_audio_state, td->act_audio_mode,
                            td->act_sample_rate, td->act_num_channels, td->act_preemph);
                        fprintf(log_file, "ses%d: new hdmi audio_state: %d, audio_format: %d, rate: %d, channels: %d, preemph: %d\n",
                            td->session_idx, td->hdmi_audio_state, td->hdmi_audio_mode,
                            td->hdmi_sample_rate, td->hdmi_num_channels, td->hdmi_preemph);

                        stop_rec_thread(td->session_idx);

                        td->act_audio_state = td->hdmi_audio_state;
                        td->act_audio_mode = td->hdmi_audio_mode;
                        td->act_sample_rate = td->hdmi_sample_rate;
                        td->act_num_channels = td->hdmi_num_channels;

                        if (td->hdmi_audio_state)
                            start_rec_thread(td->session_idx);
                        }
                } else {
                    if ((td->spdif_arc_audio_state == 2) ||
                        (td->act_preemph != td->spdif_arc_preemph) ||
                        (td->is_loopback && td->loopback.act_codec_format != td->loopback.new_codec_format)) {
                        fprintf(log_file, "ses%d: old     audio_state: %d, audio_format: %d, rate: %d, channels: %d, preemph: %d\n",
                            td->session_idx, td->act_audio_state, td->act_audio_mode,
                            td->act_sample_rate, td->act_num_channels, td->act_preemph);
                        fprintf(log_file, "ses%d: new arc audio_state: %d, audio_format: %d, rate: %d, channels: %d, preemph: %d\n",
                            td->session_idx, td->spdif_arc_audio_state, td->spdif_arc_audio_mode,
                            td->spdif_arc_sample_rate, td->spdif_arc_num_channels, td->spdif_arc_preemph);

                        stop_rec_thread(td->session_idx);

                        td->act_audio_state = 1;
                        td->act_audio_mode = td->spdif_arc_audio_mode;
                        td->act_sample_rate = td->spdif_arc_sample_rate;
                        td->act_num_channels = td->spdif_arc_num_channels;
                        td->act_preemph = td->spdif_arc_preemph;

                        start_rec_thread(td->session_idx);
                    }
                }
            }
            break;
        }
        td++;
    }
}

int poll_event_init()
{
    struct sockaddr_nl sock_addr;
    int sz = (64*1024);
    int soc;

    memset(&sock_addr, 0, sizeof(sock_addr));
    sock_addr.nl_family = AF_NETLINK;
    sock_addr.nl_pid = getpid();
    sock_addr.nl_groups = 0xffffffff;

    soc = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
    if (soc < 0) {
        return 0;
    }

    setsockopt(soc, SOL_SOCKET, SO_RCVBUFFORCE, &sz, sizeof(sz));

    if (bind(soc, (struct sockaddr*) &sock_addr, sizeof(sock_addr)) < 0) {
        close(soc);
        return 0;
    }

    sock_event_fd = soc;

    return (soc > 0);
}

void* listen_uevent()
{
    char buffer[64*1024];
    struct pollfd fds;
    int i, count;
    int j;
    char *dev_path = NULL;
    char *switch_state = NULL;
    char *switch_name = NULL;
    int audio_changed;

    input_restart_check();

    while(!stop_test) {

        fds.fd = sock_event_fd;
        fds.events = POLLIN;
        fds.revents = 0;
        i = poll(&fds, 1, 5); /* wait 5 msec */

        audio_changed = 0;
        if (i > 0 && (fds.revents & POLLIN)) {
            count = recv(sock_event_fd, buffer, (64*1024), 0 );
            if (count > 0) {
                buffer[count] = '\0';
                j = 0;
                while(j < count) {
                    if (strncmp(&buffer[j], "DEVPATH=", 8) == 0) {
                        dev_path = &buffer[j+8];
                        j += 8;
                        continue;
                    }
                    for (i=0; i<num_sessions; i++) {
                        if (tdata[i].input_device == AUDIO_DEVICE_IN_SPDIF) {
                            if (strncmp(&buffer[j], "PRI_SPDIF_TX=MEDIA_CONFIG_CHANGE", strlen("PRI_SPDIF_TX=MEDIA_CONFIG_CHANGE")) == 0) {
                                audio_changed = 1;
                                ALOGI("AUDIO CHANGE EVENT: %s\n", &buffer[j]);
                                j += strlen("PRI_SPDIF_TX=MEDIA_CONFIG_CHANGE");
                                continue;
                            }
                        } else if (tdata[i].input_device == AUDIO_DEVICE_IN_HDMI) {
                            if (strncmp(&buffer[j], "EP92EVT_AUDIO=MEDIA_CONFIG_CHANGE", strlen("EP92EVT_AUDIO=MEDIA_CONFIG_CHANGE")) == 0) {
                                audio_changed = 1;
                                ALOGI("AUDIO CHANGE EVENT: %s\n", &buffer[j]);
                                j += strlen("EP92EVT_AUDIO=MEDIA_CONFIG_CHANGE");
                                continue;
                            } else if (strncmp(&buffer[j], "EP92EVT_ARC_EN=ON", strlen("EP92EVT_ARC_EN=ON")) == 0) {
                                audio_changed = 1;
                                ALOGI("AUDIO CHANGE EVENT: %s\n", &buffer[j]);
                                j += strlen("EP92EVT_ARC_EN=ON");
                                continue;
                            } else if (strncmp(&buffer[j], "EP92EVT_ARC_EN=OFF", strlen("EP92EVT_ARC_EN=OFF")) == 0) {
                                audio_changed = 1;
                                ALOGI("AUDIO CHANGE EVENT: %s\n", &buffer[j]);
                                j += strlen("EP92EVT_ARC_EN=OFF");
                                continue;
                            } else if (strncmp(&buffer[j], "SEC_SPDIF_TX=MEDIA_CONFIG_CHANGE", strlen("SEC_SPDIF_TX=MEDIA_CONFIG_CHANGE")) == 0) {
                                audio_changed = 1;
                                ALOGI("AUDIO CHANGE EVENT: %s\n", &buffer[j]);
                                j += strlen("SEC_SPDIF_TX=MEDIA_CONFIG_CHANGE");
                                continue;
                            } else if (strncmp(&buffer[j], "EP92EVT_", 8) == 0) {
                                ALOGI("EVENT: %s\n", &buffer[j]);
                                j += 8;
                                continue;
                            }
                        }
                    }
                    j++;
                }
            }
        } else {
            ALOGV("NO Data\n");
        }

        /* we get here with poll timeout each 5ms */
        if (tdata[0].is_loopback) {
            if (tdata[0].loopback.act_codec_format != tdata[0].loopback.new_codec_format) {
                audio_changed = 1;
            }
        }
        if (audio_changed)
            input_restart_check();
    }

    for (i=0; i<num_sessions; i++)
        stop_rec_thread(i);
}

void fill_default_params(struct test_data *td) {
    memset(td, 0, sizeof(struct test_data));

    td->input_device = AUDIO_DEVICE_IN_SPDIF;
    td->bit_width = 24;
    td->source = AUDIO_SOURCE_UNPROCESSED;
    td->record_length = 8 /*sec*/;

    td->handle = 0x99A;

    td->is_loopback = 0;
    td->sink_device = AUDIO_DEVICE_OUT_SPEAKER;
    td->loopback.ch_mask = AUDIO_CHANNEL_OUT_STEREO;
    td->loopback.gain = 1.0f;
    td->loopback.patch_handle = AUDIO_PATCH_HANDLE_NONE;
    td->loopback.act_codec_format = AUDIO_FORMAT_MAT;
    td->loopback.new_codec_format = AUDIO_FORMAT_MAT;
}
int set_metadata_av_window_mat(qahw_module_handle_t *hw_module,
                               audio_patch_handle_t handle)
{
    qahw_loopback_param_payload payload;
    int ret = 0;

    fprintf(log_file, "Set the AV sync meta data params using qahw_loopback_set_param_data\n");

    payload.render_window_params.render_ws = 0xFFFFFFFFFFFE7960;
    payload.render_window_params.render_we = 0x00000000000186A0;

    ret = qahw_loopback_set_param_data(hw_module, handle,
        QAHW_PARAM_LOOPBACK_RENDER_WINDOW, &payload);

    if (ret < 0) {
        fprintf(log_file, "qahw_loopback_set_param_data av render failed with err %d\n", ret);
        goto done;
    }
done:
    return ret;
}
void usage() {
    printf(" \n Command \n");
    printf(" \n fmt_change_test <options>\n");
    printf(" \n Options\n");
    printf(" -d  --device <int>                 - spdif_in 2147549184, hdmi_in 2147483680\n");
    printf("                                      Optional Argument and Default value is spdif_in\n\n");
    printf(" -x  --device2 <int>                - create parallel session with given device\n");
    printf(" -y  --device3 <int>                - create parallel session with given device\n");
    printf(" -z  --device4 <int>                - create parallel session with given device\n");
    printf(" -b  --bits <int>                   - Bitwidth in PCM mode (16, 24 or 32), Default is 24\n\n");
    printf(" -o  --loopback <int>               - create DSP loopback session to given output device\n");
    printf("                                      Speaker 2, Line-Out 131072\n\n");
    printf(" -c  --channel_mask <int>           - loopback output channel configuration\n\n");
    printf(" -v  --volume <float>               - loopback volume configuration\n\n");
    printf(" -t  --recording-time <in seconds>  - Time duration for the recording\n\n");
    printf(" -l  --log-file <FILEPATH>          - File path for debug msg, to print\n");
    printf("                                      on console use stdout or 1 \n\n");
    printf(" -h  --help                         - Show this help\n\n");
    printf(" \n Examples \n");
    printf(" fmt_change_test                          -> start a recording stream with default configurations\n\n");
    printf(" fmt_change_test -d 2147483680 -t 20      -> start a recording session, with device hdmi_in\n");
    printf(" fmt_change_test -d 2147483680 -x 2147549184 -> start two recording session, hdmi_in and spdif_in\n");
    printf(" fmt_change_test -d 2147483680 -o 2 -t 20 -> start a loopback session, hdmi_in to speaker,\n");
    printf("                                             record data for 20 secs.\n\n");
}

static void qti_audio_server_death_notify_cb(void *ctxt)
{
    int i = 0;

    fprintf(log_file, "qas died\n");
    fprintf(stderr, "qas died\n");
    stop_test = true;

    for (i=0; i<MAX_RECORD_SESSIONS; i++)
        stop_record[i] = true;
}

int main(int argc, char* argv[])
{
    qahw_module_handle_t *qahw_mod_handle;
    const  char *mod_name = "audio.primary";

    char log_filename[256] = "stdout";
    int i;
    int ret = -1;

    log_file = stdout;
    num_sessions = 1;
    for (i=0; i<MAX_RECORD_SESSIONS; i++)
        fill_default_params(&tdata[i]);

    struct option long_options[] = {
        /* These options set a flag. */
        {"device",          required_argument,    0, 'd'},
        {"device2",         required_argument,    0, 'x'},
        {"device3",         required_argument,    0, 'y'},
        {"device4",         required_argument,    0, 'z'},
        {"loopback",        required_argument,    0, 'o'},
        {"bits",            required_argument,    0, 'b'},
        {"channel_mask",    required_argument,    0, 'c'},
        {"volume",          required_argument,    0, 'v'},
        {"recording-time",  required_argument,    0, 't'},
        {"log-file",        required_argument,    0, 'l'},
        {"help",            no_argument,          0, 'h'},
        {0, 0, 0, 0}
    };

    int opt = 0;
    int option_index = 0;
    while ((opt = getopt_long(argc,
                              argv,
                              "-d:x:y:z:o:b:c:v:t:l:h",
                              long_options,
                              &option_index)) != -1) {
        switch (opt) {
            case 'd':
                tdata[0].input_device = atoll(optarg);
                break;
            case 'x':
            case 'y':
            case 'z':
                tdata[num_sessions].input_device = atoll(optarg);
                tdata[num_sessions].session_idx = num_sessions;
                tdata[num_sessions].handle = tdata[0].handle + num_sessions;
                num_sessions++;
                break;
            case 'o':
                tdata[0].is_loopback = 1;
                tdata[0].sink_device = atoll(optarg);
                break;
            case 'b':
                for (i=0; i<MAX_RECORD_SESSIONS; i++)
                    tdata[i].bit_width = atoll(optarg);
                break;
            case 'c':
                tdata[0].loopback.ch_mask = atoll(optarg);
                break;
            case 'v':
                tdata[0].loopback.gain = atof(optarg);
                break;
            case 't':
                tdata[0].record_length = atoi(optarg);
                break;
            case 'l':
                snprintf(log_filename, sizeof(log_filename), "%s", optarg);
                break;
            case 'h':
            default:
                usage();
                return 0;
                break;
        }
    }
    fprintf(log_file, "registering qas callback\n");
    qahw_register_qas_death_notify_cb((audio_error_callback)qti_audio_server_death_notify_cb, context);

    if (tdata[0].is_loopback) {
        fprintf(log_file, "Loopback source %#x, sink %#x\n", tdata[0].input_device, tdata[0].sink_device);
    }

    for (i=0; i<num_sessions; i++) {
        switch (tdata[i].input_device) {
        case AUDIO_DEVICE_IN_SPDIF:
            break;
        case AUDIO_DEVICE_IN_HDMI:
            break;
        default:
            fprintf(log_file, "device %d not supported\n", tdata[i].input_device);
            return -1;
        }
    }

    switch (tdata[0].bit_width) {
    case 16:
    case 24:
    case 32:
        break;
    default:
        fprintf(log_file, "bitwidth %d not supported\n", tdata[0].bit_width);
        return -1;
    }

    qahw_mod_handle = qahw_load_module(mod_name);
    if(qahw_mod_handle == NULL) {
        fprintf(log_file, " qahw_load_module failed");
        return -1;
    }
    fprintf(log_file, " Starting audio recording test. \n");
    if (strcasecmp(log_filename, "stdout") && strcasecmp(log_filename, "1")) {
        if ((log_file = fopen(log_filename,"wb"))== NULL) {
            fprintf(stderr, "Cannot open log file %s\n", log_filename);
            /* continue to log to std out */
            log_file = stdout;
        }
    }

    for (i=0; i<num_sessions; i++)
        tdata[i].qahw_mod_handle = qahw_mod_handle;

    /* Register the SIGINT to close the App properly */
    if (signal(SIGINT, stop_signal_handler) == SIG_ERR)
        fprintf(log_file, "Failed to register SIGINT:%d\n", errno);

    /* Register the SIGTERM to close the App properly */
    if (signal(SIGTERM, stop_signal_handler) == SIG_ERR)
        fprintf(log_file, "Failed to register SIGTERM:%d\n", errno);

    time_t start_time = time(0);
    double time_elapsed = 0;

    pthread_attr_init(&tdata[0].poll_event_attr);
    pthread_attr_setdetachstate(&tdata[0].poll_event_attr, PTHREAD_CREATE_JOINABLE);
    poll_event_init();
    pthread_create(&tdata[0].poll_event_th, &tdata[0].poll_event_attr,
                       (void *) listen_uevent, NULL);

    while(true && !stop_test) {
        time_elapsed = difftime(time(0), start_time);
        if (tdata[0].record_length && (time_elapsed > tdata[0].record_length)) {
            fprintf(log_file, "\n Test completed.\n");
            stop_test = true;
            break;
        }
    }

    fprintf(log_file, "\n Stop test \n");

    pthread_join(tdata[0].poll_event_th, NULL);

    fprintf(log_file, "\n Unload HAL\n");

    ret = qahw_unload_module(qahw_mod_handle);
    if (ret) {
        fprintf(log_file, "could not unload hal %d\n", ret);
    }

    fprintf(log_file, "Done with hal record test\n");
    if (log_file != stdout) {
        if (log_file) {
          fclose(log_file);
          log_file = NULL;
        }
    }

    return 0;
}
