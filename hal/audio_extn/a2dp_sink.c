/*
* Copyright (c) 2015-2021, The Linux Foundation. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above
*       copyright notice, this list of conditions and the following
*       disclaimer in the documentation and/or other materials provided
*       with the distribution.
*     * Neither the name of The Linux Foundation nor the names of its
*       contributors may be used to endorse or promote products derived
*       from this software without specific prior written permission.
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
#define LOG_TAG "split_a2dp_sink"
/*#define LOG_NDEBUG 0*/
#define LOG_NDDEBUG 0
#include <errno.h>
#include <cutils/log.h>
#include <dlfcn.h>
#include "audio_hw.h"
#include "platform.h"
#include "platform_api.h"
#include "audio_extn.h"
#include <stdlib.h>
#include <cutils/str_parms.h>
#include <hardware/audio.h>
#include <hardware/hardware.h>
#include <cutils/properties.h>

#ifdef DYNAMIC_LOG_ENABLED
#include <log_xml_parser.h>
#define LOG_MASK HAL_MOD_FILE_A2DP
#include <log_utils.h>
#endif

#define BT_IPC_SINK_LIB_NAME    "libbthost_if_sink.so"
#define MEDIA_FMT_NONE                                     0
#define MEDIA_FMT_AAC                                      0x00010DA6
#define MEDIA_FMT_APTX                                     0x000131ff
#define MEDIA_FMT_APTX_HD                                  0x00013200
#define MEDIA_FMT_APTX_AD                                  0x00013204
#define MEDIA_FMT_SBC                                      0x00010BF2
#define MEDIA_FMT_CELT                                     0x00013221
#define MEDIA_FMT_LDAC                                     0x00013224
#define MEDIA_FMT_MP3                                      0x00010BE9
#define MEDIA_FMT_AAC_AOT_LC                               2
#define MEDIA_FMT_AAC_AOT_SBR                              5
#define MEDIA_FMT_AAC_AOT_PS                               29
#define MEDIA_FMT_SBC_CHANNEL_MODE_MONO                    1
#define MEDIA_FMT_SBC_CHANNEL_MODE_STEREO                  2
#define MEDIA_FMT_SBC_CHANNEL_MODE_DUAL_MONO               8
#define MEDIA_FMT_SBC_CHANNEL_MODE_JOINT_STEREO            9
#define MEDIA_FMT_SBC_ALLOCATION_METHOD_LOUDNESS           0
#define MEDIA_FMT_SBC_ALLOCATION_METHOD_SNR                1
#define MEDIA_FMT_APTX_AD_CHANNELS_MONO                    1
#define MEDIA_FMT_APTX_AD_CHANNELS_TWS_STEREO              4
#define MEDIA_FMT_APTX_AD_CHANNELS_JOINT_STEREO            0
#define MEDIA_FMT_APTX_AD_CHANNELS_TWS_MONO                2

#define MIXER_SINK_DEC_CONFIG_BLOCK       "SLIM_9_TX Decoder Config"
#define MIXER_DEC_BIT_FORMAT       "AFE Output Bit Format"
#define MIXER_SINK_SAMPLE_RATE     "BT_TX SampleRate"
#define MIXER_AFE_SINK_CHANNELS    "AFE Output Channels"
#define MIXER_ADM_CHANNELS    "SLIM9_TX ADM Channels"
#define MIXER_TTP_GEN_ENABLE       "TTP Enable"
#define MIXER_TTP_GEN_CONFIG       "AFE TTP config"
#define DEFAULT_SINK_LATENCY_SBC       140
#define DEFAULT_SINK_LATENCY_APTX      160
#define DEFAULT_SINK_LATENCY_APTX_HD   180
#define DEFAULT_SINK_LATENCY_APTX_AD   180
#define DEFAULT_SINK_LATENCY_AAC       180
//To Do: Fine Tune Default CELT/LDAC Latency.
#define DEFAULT_SINK_LATENCY_CELT      180
#define DEFAULT_SINK_LATENCY_LDAC      180
#define MAX_TTP_OFFSET_PAIRS 4

#define ENABLE_TTP_GENERATOR 0x01
#define TTP_GEN_CONFIG_PARAM 0x00030D40
/*
 * Below enum values are extended from audio_base.h to
 * to keep encoder and decoder type local to bthost_ipc
 * and audio_hal as these are intended only for handshake
 * between IPC lib and Audio HAL.
 */
typedef enum {
    CODEC_TYPE_INVALID = AUDIO_FORMAT_INVALID, // 0xFFFFFFFFUL
    CODEC_TYPE_AAC = AUDIO_FORMAT_AAC, // 0x04000000UL
    CODEC_TYPE_SBC = AUDIO_FORMAT_SBC, // 0x1F000000UL
    CODEC_TYPE_APTX = AUDIO_FORMAT_APTX, // 0x20000000UL
    CODEC_TYPE_APTX_HD = AUDIO_FORMAT_APTX_HD, // 0x21000000UL
    CODEC_TYPE_APTX_DUAL_MONO = 570425344u, // 0x22000000UL
    CODEC_TYPE_LDAC = AUDIO_FORMAT_LDAC, // 0x23000000UL
    CODEC_TYPE_CELT = 603979776u, // 0x24000000UL
    CODEC_TYPE_APTX_AD = 620756992u, // 0x25000000UL
} codec_t;

typedef enum {
    APTX_48 = 0x0,  // 48 KHz default
    APTX_44_1 = 0x1, // 44.1kHz
} dec_aptx_s_rate;

typedef int (*audio_sink_start_t)(void);
typedef int (*audio_sink_stop_t)(void);
typedef void * (*audio_get_dec_config_t)(codec_t *codec_type);
typedef void * (*audio_sink_session_setup_complete_t)(uint64_t system_latency);
typedef int (*audio_sink_check_a2dp_ready_t)(void);
static void open_a2dp_sink();

/* structure used to  update a2dp state machine
 * to communicate IPC library
 * to store DSP decoder configuration information
 */
struct a2dp_sink_data {
    struct audio_device *adev;
    void *bt_lib_sink_handle;
    audio_sink_start_t audio_sink_start;
    audio_sink_stop_t audio_sink_stop;
    audio_get_dec_config_t audio_get_dec_config;
    audio_sink_session_setup_complete_t audio_sink_session_setup_complete;
    audio_sink_check_a2dp_ready_t audio_sink_check_a2dp_ready;
    codec_t bt_decoder_format;
    uint32_t dec_sampling_rate;
    uint32_t dec_channels;
    bool a2dp_sink_started;
    int  a2dp_sink_total_active_session_requests;
    bool enable_ttp;
};

struct a2dp_sink_data a2dp_sink;

typedef struct audio_aac_decoder_config_t audio_aac_decoder_config_t;
struct audio_aac_decoder_config_t {
    uint16_t      aac_fmt_flag; /* LATM*/
    uint16_t      audio_object_type; /* LC */
    uint16_t      channels; /* Stereo */
    uint16_t      total_size_of_pce_bits; /* 0 - only for channel conf PCE */
    uint32_t      sampling_rate; /* 8k, 11.025k, 12k, 16k, 22.05k, 24k, 32k,
                                  44.1k, 48k, 64k, 88.2k, 96k */
} __attribute__ ((packed));

typedef struct audio_sbc_decoder_config_t audio_sbc_decoder_config_t;
struct audio_sbc_decoder_config_t {
    uint16_t      channels; /* Mono, Stereo */
    uint32_t      sampling_rate; /* 8k, 11.025k, 12k, 16k, 22.05k, 24k, 32k,
                                  44.1k, 48k, 64k, 88.2k, 96k */
} __attribute__ ((packed));

typedef struct audio_aptx_ad_decoder_config_t audio_aptx_ad_decoder_config_t;
struct audio_aptx_ad_decoder_config_t {
    uint32_t      sampling_rate; /* 0x0(48k), 0x1(44.1k) */
} __attribute__ ((packed));

typedef struct audio_aptx_classic_decoder_config_t audio_aptx_classic_decoder_config_t;
struct audio_aptx_classic_decoder_config_t {
    uint32_t      sampling_rate; /* 0x0(48k), 0x1(44.1k) */
} __attribute__ ((packed));

typedef struct audio_aptx_hd_decoder_config_t audio_aptx_hd_decoder_config_t;
struct audio_aptx_hd_decoder_config_t {
    uint32_t      sampling_rate; /* 0x0(48k), 0x1(44.1k) */
} __attribute__ ((packed));

/* AAC decoder configuration structure. */
typedef struct aac_dec_cfg_t aac_dec_cfg_t;
struct aac_dec_cfg_t {
    uint32_t dec_format;
    audio_aac_decoder_config_t data;
} __attribute__ ((packed));

/* SBC decoder configuration structure. */
typedef struct sbc_dec_cfg_t sbc_dec_cfg_t;
struct sbc_dec_cfg_t {
    uint32_t dec_format;
    audio_sbc_decoder_config_t data;
} __attribute__ ((packed));

/* Aptx Adavptive decoder configuration structure. */
typedef struct aptx_ad_dec_cfg_t aptx_ad_dec_cfg_t;
struct aptx_ad_dec_cfg_t {
    uint32_t dec_format;
    audio_aptx_ad_decoder_config_t data;
} __attribute__ ((packed));

/* Aptx Classic decoder configuration structure. */
typedef struct aptx_classic_dec_cfg_t aptx_classic_dec_cfg_t;
struct aptx_classic_dec_cfg_t {
    uint32_t dec_format;
    audio_aptx_classic_decoder_config_t data;
} __attribute__ ((packed));

/* Aptx HD decoder configuration structure. */
typedef struct aptx_hd_dec_cfg_t aptx_hd_dec_cfg_t;
struct aptx_hd_dec_cfg_t {
    uint32_t dec_format;
    audio_aptx_hd_decoder_config_t data;
} __attribute__ ((packed));

/* Information about BT AAC decoder configuration
 * This data is used between audio HAL module and
 * BT IPC library to configure DSP decoder
 */
typedef struct {
    uint16_t      aac_fmt_flag; /* LATM*/
    uint16_t      audio_object_type; /* LC */
    uint16_t      channels; /* Stereo */
    uint16_t      total_size_of_pce_bits; /* 0 - only for channel conf PCE */
    uint32_t      sampling_rate; /* 8k, 11.025k, 12k, 16k, 22.05k, 24k, 32k,
                                  44.1k, 48k, 64k, 88.2k, 96k */
} audio_aac_dec_config_t;

/* Information about BT SBC decoder configuration
 * This data is used between audio HAL module and
 * BT IPC library to configure DSP decoder
 */
typedef struct {
    uint16_t      channels; /* Mono, Stereo */
    uint32_t      sampling_rate; /* 8k, 11.025k, 12k, 16k, 22.05k, 24k, 32k,
                                  44.1k, 48k, 64k, 88.2k, 96k */
} audio_sbc_dec_config_t;

typedef struct {
    uint32_t sampling_rate; /* 0x0(48k), 0x1(44.1k) */
    uint8_t channel_mode; /* Mono, Stereo */
} audio_aptx_ad_dec_config_t;

typedef struct {
    uint32_t sampling_rate; /* 0x0(48k), 0x1(44.1k) */
    uint8_t channel_mode; /* Stereo */
} audio_aptx_classic_dec_config_t;

typedef struct {
    uint32_t      sampling_rate; /* 0x0(48k), 0x1(44.1k) */
    uint8_t       channel_mode; /* Stereo */
} audio_aptx_hd_dec_config_t;

/* structure to enable TTP generator in AFE */
struct afe_ttp_gen_enable {
    uint16_t enable;
    uint16_t reserved;
} __attribute__ ((packed));

/* structure to set SSRC offset in TTP */
struct afe_ttp_ssrc_offset_pair {
    uint32_t ssrc;
    uint32_t offset;
} __attribute__ ((packed));

static struct afe_ttp_ssrc_offset_pair ssrc_ttp[MAX_TTP_OFFSET_PAIRS] = {
    {0xa1, 0x30D40},
    {0xa2, 0x30D40},
    {0xa3, 0x30D40},
    {0xa4, 0x30D40},
};

/* structure to set TTP generator config*/
struct afe_ttp_gen_cfg_num_offset
{
    uint32_t ttp_offset_default;   /* TTP offset uses for all other cases
                                      where no valid SSRC is received */
    uint32_t settling_time;        /* If settling_mode==0x00: time in [us]
                                      after first received packet until
                                      packets are no longer dropped */
    uint16_t settling_mode;        /* 0x00(Drop), 0x01(Settle) */
    uint16_t num_ssrc_offsets;     /* Number of SSRC/TTPOFFSET pairs to follow */
    struct afe_ttp_ssrc_offset_pair ssrc_ttp_offset[MAX_TTP_OFFSET_PAIRS]; /* Array of ssrc/offset pairs */
} __attribute__ ((packed));

/*********** END of DSP configurable structures ********************/

/* API to open BT IPC library to start IPC communication for BT Sink*/
static void open_a2dp_sink()
{
    ALOGD(" Open A2DP input start ");
    if (a2dp_sink.bt_lib_sink_handle == NULL){
        ALOGD(" Requesting for BT lib handle");
        a2dp_sink.bt_lib_sink_handle = dlopen(BT_IPC_SINK_LIB_NAME, RTLD_NOW);

        if (a2dp_sink.bt_lib_sink_handle == NULL) {
            ALOGE("%s: DLOPEN failed for %s", __func__, BT_IPC_SINK_LIB_NAME);
        } else {
            a2dp_sink.audio_sink_start = (audio_sink_start_t)
                          dlsym(a2dp_sink.bt_lib_sink_handle, "audio_sink_start_capture");
            a2dp_sink.audio_get_dec_config = (audio_get_dec_config_t)
                          dlsym(a2dp_sink.bt_lib_sink_handle, "audio_get_decoder_config");
            a2dp_sink.audio_sink_stop = (audio_sink_stop_t)
                          dlsym(a2dp_sink.bt_lib_sink_handle, "audio_sink_stop_capture");
            a2dp_sink.audio_sink_check_a2dp_ready = (audio_sink_check_a2dp_ready_t)
                        dlsym(a2dp_sink.bt_lib_sink_handle,"audio_sink_check_a2dp_ready");
            a2dp_sink.audio_sink_session_setup_complete = (audio_sink_session_setup_complete_t)
                          dlsym(a2dp_sink.bt_lib_sink_handle, "audio_sink_session_setup_complete");
        }
    }
}

static int close_a2dp_input()
{
    ALOGV("%s\n",__func__);

    if (!(a2dp_sink.bt_lib_sink_handle)) {
        ALOGE("a2dp sink handle is not identified, Ignoring close request");
        return -ENOSYS;
    }

    a2dp_sink.a2dp_sink_started = false;
    a2dp_sink.a2dp_sink_total_active_session_requests = 0;
    a2dp_sink.bt_decoder_format = CODEC_TYPE_INVALID;
    a2dp_sink.dec_sampling_rate = 48000;
    a2dp_sink.dec_channels = 2;

    return 0;
}

static bool a2dp_set_backend_cfg()
{
    char *rate_str = NULL, *channels = NULL;
    uint32_t sampling_rate = a2dp_sink.dec_sampling_rate;
    struct mixer_ctl *ctl_sample_rate = NULL, *ctrl_channels = NULL;
    bool is_configured = false;

    //For SBC and AAC decoder open slimbus port at
    //96Khz for both 48Khz and 44.1Khz input.
    if (((a2dp_sink.bt_decoder_format == CODEC_TYPE_SBC) ||
         (a2dp_sink.bt_decoder_format == CODEC_TYPE_AAC) ||
         (a2dp_sink.bt_decoder_format == CODEC_TYPE_APTX_AD) ||
         (a2dp_sink.bt_decoder_format == CODEC_TYPE_APTX) ||
         (a2dp_sink.bt_decoder_format == CODEC_TYPE_APTX_HD)) &&
        (sampling_rate == 48000 || sampling_rate == 44100 )) {
        sampling_rate = 96000;
    }

    //Configure backend sampling rate
    switch (sampling_rate) {
    case 44100:
        rate_str = "KHZ_44P1";
        break;
    case 88200:
        rate_str = "KHZ_88P2";
        break;
    case 96000:
        rate_str = "KHZ_96";
        break;
    case 48000:
    default:
        rate_str = "KHZ_48";
        break;
    }

    ALOGD("%s: set sink backend sample rate =%s", __func__, rate_str);
    ctl_sample_rate = mixer_get_ctl_by_name(a2dp_sink.adev->mixer,
                                            MIXER_SINK_SAMPLE_RATE);
    if (ctl_sample_rate) {
        if (mixer_ctl_set_enum_by_string(ctl_sample_rate, rate_str) != 0) {
            ALOGE("%s: Failed to set backend sample rate = %s", __func__, rate_str);
            is_configured = false;
            goto fail;
        }
    }

    switch (a2dp_sink.dec_channels) {
    case 1:
         channels = "One";
         break;
    case 2:
    default:
        channels = "Two";
        break;
    }

    ALOGD("%s: set afe dec channels =%s", __func__, channels);
    ctrl_channels = mixer_get_ctl_by_name(a2dp_sink.adev->mixer,
                                          MIXER_AFE_SINK_CHANNELS);
    if (!ctrl_channels) {
        ALOGE(" ERROR AFE channels mixer control not identified");
    } else {
        if (mixer_ctl_set_enum_by_string(ctrl_channels, channels) != 0) {
            ALOGE("%s: Failed to set AFE channels =%s", __func__, channels);
            is_configured = false;
            goto fail;
        }
    }
    ctrl_channels = mixer_get_ctl_by_name(a2dp_sink.adev->mixer,
                                          MIXER_ADM_CHANNELS);
    if (!ctrl_channels) {
        ALOGE(" ERROR ADM channels mixer control not identified");
    } else {
        if (mixer_ctl_set_enum_by_string(ctrl_channels, channels) != 0) {
            ALOGE("%s: Failed to set ADM channels =%s", __func__, channels);
            is_configured = false;
            goto fail;
        }
    }
    is_configured = true;
fail:
    return is_configured;
}

bool configure_aac_dec_format(audio_aac_dec_config_t *aac_bt_cfg)
{
    struct mixer_ctl *ctl_dec_data = NULL, *ctrl_bit_format = NULL;
    struct aac_dec_cfg_t aac_dsp_cfg;
    bool is_configured = false;
    int ret = 0;

    if (aac_bt_cfg == NULL)
        return false;

    ctl_dec_data = mixer_get_ctl_by_name(a2dp_sink.adev->mixer, MIXER_SINK_DEC_CONFIG_BLOCK);
    if (!ctl_dec_data) {
        ALOGE(" ERROR  a2dp decoder CONFIG data mixer control not identified");
        is_configured = false;
        goto fail;
    }

    memset(&aac_dsp_cfg, 0x0, sizeof(struct aac_dec_cfg_t));
    aac_dsp_cfg.dec_format = MEDIA_FMT_AAC;
    aac_dsp_cfg.data.aac_fmt_flag = aac_bt_cfg->aac_fmt_flag;
    aac_dsp_cfg.data.channels = aac_bt_cfg->channels;
    switch(aac_bt_cfg->audio_object_type) {
    case 0:
        aac_dsp_cfg.data.audio_object_type = MEDIA_FMT_AAC_AOT_LC;
        break;
    case 2:
        aac_dsp_cfg.data.audio_object_type = MEDIA_FMT_AAC_AOT_PS;
        break;
    case 1:
    default:
        aac_dsp_cfg.data.audio_object_type = MEDIA_FMT_AAC_AOT_SBR;
        break;
    }
    aac_dsp_cfg.data.total_size_of_pce_bits = aac_bt_cfg->total_size_of_pce_bits;
    aac_dsp_cfg.data.sampling_rate = aac_bt_cfg->sampling_rate;
    ret = mixer_ctl_set_array(ctl_dec_data, (void *)&aac_dsp_cfg,
                              sizeof(struct aac_dec_cfg_t));
    if (ret != 0) {
        ALOGE("%s: failed to set AAC decoder config", __func__);
        is_configured = false;
        goto fail;
    }

    ctrl_bit_format = mixer_get_ctl_by_name(a2dp_sink.adev->mixer,
                                            MIXER_DEC_BIT_FORMAT);
    if (!ctrl_bit_format) {
        ALOGE(" ERROR Dec bit format mixer control not identified");
        is_configured = false;
        goto fail;
    }
    ret = mixer_ctl_set_enum_by_string(ctrl_bit_format, "S16_LE");
    if (ret != 0) {
        ALOGE("%s: Failed to set bit format to decoder", __func__);
        is_configured = false;
        goto fail;
    }

    is_configured = true;
    a2dp_sink.bt_decoder_format = CODEC_TYPE_AAC;
    a2dp_sink.dec_channels = aac_dsp_cfg.data.channels;
    a2dp_sink.dec_sampling_rate = aac_dsp_cfg.data.sampling_rate;
    ALOGV("Successfully updated AAC dec format with sampling_rate: %d channels:%d",
           aac_dsp_cfg.data.sampling_rate, aac_dsp_cfg.data.channels);
fail:
    return is_configured;
}

static int a2dp_reset_backend_cfg()
{
    const char *rate_str = "KHZ_8", *channels = "Zero";
    struct mixer_ctl *ctl_sample_rate = NULL;
    struct mixer_ctl *ctrl_channels = NULL;

    // Reset backend sampling rate
    ALOGD("%s: reset sink backend sample rate =%s", __func__, rate_str);
    ctl_sample_rate = mixer_get_ctl_by_name(a2dp_sink.adev->mixer,
                                            MIXER_SINK_SAMPLE_RATE);
    if (ctl_sample_rate) {
        if (mixer_ctl_set_enum_by_string(ctl_sample_rate, rate_str) != 0) {
            ALOGE("%s: Failed to reset backend sample rate = %s", __func__, rate_str);
            return -ENOSYS;
        }
    }

    // Reset AFE input channels
    ALOGD("%s: reset afe sink channels =%s", __func__, channels);
    ctrl_channels = mixer_get_ctl_by_name(a2dp_sink.adev->mixer,
                                            MIXER_AFE_SINK_CHANNELS);
    if (!ctrl_channels) {
        ALOGE("%s: ERROR AFE input channels mixer control not identifed", __func__);
        return -ENOSYS;
    }
    if (mixer_ctl_set_enum_by_string(ctrl_channels, channels) != 0) {
        ALOGE("%s: Failed to reset AFE in channels = %d", __func__, a2dp_sink.dec_channels);
        return -ENOSYS;
    }

    return 0;
}

bool configure_sbc_dec_format(audio_sbc_dec_config_t *sbc_bt_cfg)
{
    struct mixer_ctl *ctl_dec_data = NULL, *ctrl_bit_format = NULL;
    struct sbc_dec_cfg_t sbc_dsp_cfg;
    bool is_configured = false;
    int ret = 0;

    if (sbc_bt_cfg == NULL)
        goto fail;

    ctl_dec_data = mixer_get_ctl_by_name(a2dp_sink.adev->mixer, MIXER_SINK_DEC_CONFIG_BLOCK);
    if (!ctl_dec_data) {
        ALOGE(" ERROR  a2dp decoder CONFIG data mixer control not identified");
        is_configured = false;
        goto fail;
    }

    memset(&sbc_dsp_cfg, 0x0, sizeof(struct sbc_dec_cfg_t));
    sbc_dsp_cfg.dec_format = MEDIA_FMT_SBC;
    sbc_dsp_cfg.data.channels = sbc_bt_cfg->channels;
    sbc_dsp_cfg.data.sampling_rate = sbc_bt_cfg->sampling_rate;
    ret = mixer_ctl_set_array(ctl_dec_data, (void *)&sbc_dsp_cfg,
                              sizeof(struct sbc_dec_cfg_t));

    if (ret != 0) {
        ALOGE("%s: failed to set SBC decoder config", __func__);
        is_configured = false;
        goto fail;
    }

    ctrl_bit_format = mixer_get_ctl_by_name(a2dp_sink.adev->mixer,
                                            MIXER_DEC_BIT_FORMAT);
    if (!ctrl_bit_format) {
        ALOGE(" ERROR Dec bit format mixer control not identified");
        is_configured = false;
        goto fail;
    }
    ret = mixer_ctl_set_enum_by_string(ctrl_bit_format, "S16_LE");
    if (ret != 0) {
        ALOGE("%s: Failed to set bit format to decoder", __func__);
        is_configured = false;
        goto fail;
    }

    is_configured = true;
    a2dp_sink.bt_decoder_format = CODEC_TYPE_SBC;
    if (sbc_dsp_cfg.data.channels == MEDIA_FMT_SBC_CHANNEL_MODE_MONO)
        a2dp_sink.dec_channels = 1;
    else
        a2dp_sink.dec_channels = 2;
    a2dp_sink.dec_sampling_rate = sbc_dsp_cfg.data.sampling_rate;
    ALOGV("Successfully updated SBC dec format");
fail:
    return is_configured;
}

static bool configure_aptx_ad_dec_format(audio_aptx_ad_dec_config_t *aptx_ad_bt_cfg)
{
    struct mixer_ctl *ctl_dec_data = NULL, *ctrl_bit_format = NULL;
    struct aptx_ad_dec_cfg_t aptx_ad_dsp_cfg;
    bool is_configured = false;
    int ret = 0;

    if (aptx_ad_bt_cfg == NULL)
        goto fail;

    ctl_dec_data = mixer_get_ctl_by_name(a2dp_sink.adev->mixer, MIXER_SINK_DEC_CONFIG_BLOCK);
    if (!ctl_dec_data) {
        ALOGE(" ERROR  a2dp decoder CONFIG data mixer control not identified");
        is_configured = false;
        goto fail;
    }

    memset(&aptx_ad_dsp_cfg, 0x0, sizeof(struct aptx_ad_dec_cfg_t));
    aptx_ad_dsp_cfg.dec_format = MEDIA_FMT_APTX_AD;
    aptx_ad_dsp_cfg.data.sampling_rate = aptx_ad_bt_cfg->sampling_rate;
    ret = mixer_ctl_set_array(ctl_dec_data, (void *)&aptx_ad_dsp_cfg,
                              sizeof(struct aptx_ad_dec_cfg_t));
    if (ret != 0) {
        ALOGE("%s: failed to set Aptx ad decoder config", __func__);
        is_configured = false;
        goto fail;
    }

    ctrl_bit_format = mixer_get_ctl_by_name(a2dp_sink.adev->mixer,
                                            MIXER_DEC_BIT_FORMAT);
    if (!ctrl_bit_format) {
        ALOGE(" ERROR Dec bit format mixer control not identified");
        is_configured = false;
        goto fail;
    }

    ret = mixer_ctl_set_enum_by_string(ctrl_bit_format, "S24_LE");
    if (ret != 0) {
        ALOGE("%s: Failed to set bit format to decoder", __func__);
        is_configured = false;
        goto fail;
    }

    is_configured = true;
    a2dp_sink.bt_decoder_format = CODEC_TYPE_APTX_AD;
    switch(aptx_ad_bt_cfg->sampling_rate) {
        case APTX_44_1:
            a2dp_sink.dec_sampling_rate = 44100;
            break;
        case APTX_48:
        default:
            a2dp_sink.dec_sampling_rate = 48000;
            break;
    }
    if (aptx_ad_bt_cfg->channel_mode == MEDIA_FMT_APTX_AD_CHANNELS_MONO)
        a2dp_sink.dec_channels = 1;
    else
        a2dp_sink.dec_channels = 2;

    ALOGV("Successfully updated Aptx ad dec format");
fail:
    return is_configured;
}

static bool configure_aptx_classic_dec_format(audio_aptx_classic_dec_config_t *aptx_classic_bt_cfg)
{
   struct mixer_ctl *ctl_dec_data = NULL, *ctrl_bit_format = NULL;
   struct aptx_classic_dec_cfg_t aptx_classic_dsp_cfg;
   bool is_configured = false;
   int ret = 0;

   if (aptx_classic_bt_cfg == NULL)
       goto fail;

   ctl_dec_data = mixer_get_ctl_by_name(a2dp_sink.adev->mixer,
                                        MIXER_SINK_DEC_CONFIG_BLOCK);
   if (!ctl_dec_data) {
       ALOGE(" ERROR a2dp decoder config data mixer control not identified");
       is_configured = false;
       goto fail;
   }

   memset(&aptx_classic_dsp_cfg, 0x0, sizeof(aptx_classic_dec_cfg_t));
   aptx_classic_dsp_cfg.dec_format = MEDIA_FMT_APTX;
   aptx_classic_dsp_cfg.data.sampling_rate = aptx_classic_bt_cfg->sampling_rate;
   ret = mixer_ctl_set_array(ctl_dec_data, (void *)&aptx_classic_dsp_cfg,
                             sizeof(struct aptx_classic_dec_cfg_t));
   if (ret != 0) {
       ALOGE("%s: failed to set aptx classic decoder config", __func__);
       is_configured = false;
       goto fail;
   }

   ctrl_bit_format = mixer_get_ctl_by_name(a2dp_sink.adev->mixer,
                                           MIXER_DEC_BIT_FORMAT);
    if (!ctrl_bit_format) {
        ALOGE(" ERROR Dec bit format mixer control not identified");
        is_configured = false;
        goto fail;
    }

    ret = mixer_ctl_set_enum_by_string(ctrl_bit_format, "S16_LE");
    if (ret != 0) {
        ALOGE("%s: Failed to set bit format to decoder", __func__);
        is_configured = false;
        goto fail;
    }

    is_configured = true;
    a2dp_sink.bt_decoder_format = CODEC_TYPE_APTX;
    a2dp_sink.dec_sampling_rate = aptx_classic_bt_cfg->sampling_rate;
    a2dp_sink.dec_channels = aptx_classic_bt_cfg->channel_mode;

    ALOGV("Successfully updated Aptx classic dec format");
fail:
    return is_configured;
}

static bool configure_aptx_hd_dec_format(audio_aptx_hd_dec_config_t *aptx_hd_bt_cfg)
{
   struct mixer_ctl *ctl_dec_data = NULL, *ctrl_bit_format = NULL;
   struct aptx_hd_dec_cfg_t aptx_hd_dsp_cfg;
   bool is_configured = false;
   int ret = 0;

   if (aptx_hd_bt_cfg == NULL)
       goto fail;

   ctl_dec_data = mixer_get_ctl_by_name(a2dp_sink.adev->mixer,
                                        MIXER_SINK_DEC_CONFIG_BLOCK);
   if (!ctl_dec_data) {
       ALOGE(" ERROR a2dp decoder config data mixer control not identified");
       is_configured = false;
       goto fail;
   }

   memset(&aptx_hd_dsp_cfg, 0x0, sizeof(aptx_hd_dec_cfg_t));
   aptx_hd_dsp_cfg.dec_format = MEDIA_FMT_APTX_HD;
   aptx_hd_dsp_cfg.data.sampling_rate = aptx_hd_bt_cfg->sampling_rate;
   ret = mixer_ctl_set_array(ctl_dec_data, (void *)&aptx_hd_dsp_cfg,
                             sizeof(struct aptx_hd_dec_cfg_t));
   if (ret != 0) {
       ALOGE("%s: failed to set aptx hd decoder config", __func__);
       is_configured = false;
       goto fail;
   }

   ctrl_bit_format = mixer_get_ctl_by_name(a2dp_sink.adev->mixer,
                                           MIXER_DEC_BIT_FORMAT);
    if (!ctrl_bit_format) {
        ALOGE(" ERROR Dec bit format mixer control not identified");
        is_configured = false;
        goto fail;
    }

    ret = mixer_ctl_set_enum_by_string(ctrl_bit_format, "S24_LE");
    if (ret != 0) {
        ALOGE("%s: Failed to set bit format to decoder", __func__);
        is_configured = false;
        goto fail;
    }

    is_configured = true;
    a2dp_sink.bt_decoder_format = CODEC_TYPE_APTX_HD;
    a2dp_sink.dec_sampling_rate = aptx_hd_bt_cfg->sampling_rate;
    a2dp_sink.dec_channels = aptx_hd_bt_cfg->channel_mode;

    ALOGV("Successfully updated Aptx HD dec format");
fail:
    return is_configured;
}

/* API to configure AFE decoder in DSP */
static bool configure_a2dp_sink_decoder_format(bool *use_ttp_generator)
{
    void *codec_info = NULL;
    codec_t codec_type = CODEC_TYPE_INVALID;
    bool is_configured = false;

    if (!a2dp_sink.audio_get_dec_config) {
        ALOGE(" a2dp handle is not identified, ignoring a2dp decoder config");
        return false;
    }

    *use_ttp_generator = true;
    codec_info = a2dp_sink.audio_get_dec_config(&codec_type);
    switch(codec_type) {
        case CODEC_TYPE_SBC:
            ALOGD(" SBC decoder supported BT device");
            is_configured = configure_sbc_dec_format((audio_sbc_dec_config_t *)codec_info);
            *use_ttp_generator = false;
            break;
        case CODEC_TYPE_AAC:
            ALOGD(" AAC decoder supported BT device");
            is_configured =
              configure_aac_dec_format((audio_aac_dec_config_t *)codec_info);
            *use_ttp_generator = false;
            break;
        case CODEC_TYPE_APTX_AD:
            ALOGD(" Aptx Adaptive decoder supported BT device");
            is_configured =
              configure_aptx_ad_dec_format((audio_aptx_ad_dec_config_t *)codec_info);
            break;
        case CODEC_TYPE_APTX:
            ALOGD(" Aptx Classic decoder supported BT device");
            is_configured =
              configure_aptx_classic_dec_format((audio_aptx_classic_dec_config_t *)codec_info);
            break;
        case CODEC_TYPE_APTX_HD:
            ALOGD(" Aptx HD decoder supported BT device");
            is_configured =
              configure_aptx_hd_dec_format((audio_aptx_hd_dec_config_t *)codec_info);
            break;
        default:
            ALOGD(" Received Unsupported decoder format");
            is_configured = false;
            break;
    }

    return is_configured;
}

uint64_t audio_extn_a2dp_get_decoder_latency()
{
    uint32_t latency = 0;

    switch(a2dp_sink.bt_decoder_format) {
        case CODEC_TYPE_SBC:
            latency = DEFAULT_SINK_LATENCY_SBC;
            break;
        case CODEC_TYPE_AAC:
            latency = DEFAULT_SINK_LATENCY_AAC;
            break;
        case CODEC_TYPE_APTX_AD:
            latency = DEFAULT_SINK_LATENCY_APTX_AD;
            break;
        case CODEC_TYPE_APTX:
            latency = DEFAULT_SINK_LATENCY_APTX;
            break;
        case CODEC_TYPE_APTX_HD:
            latency = DEFAULT_SINK_LATENCY_APTX_HD;
            break;
        default:
            latency = 200;
            ALOGD("No valid decoder defined, setting latency to %dms", latency);
            break;
    }
    return (uint64_t)latency;
}

bool a2dp_send_sink_setup_complete(void) {
    uint64_t system_latency = 0;
    bool is_complete = false;

    system_latency = audio_extn_a2dp_get_decoder_latency();

    if (a2dp_sink.audio_sink_session_setup_complete(system_latency) == 0) {
        is_complete = true;
    }
    return is_complete;
}

static void disable_ttp_generator()
{
    struct afe_ttp_gen_enable ttp_gen_enable;
    struct mixer_ctl *enable_ttp = NULL;
    int ret = 0;

    ALOGV("%s: setting TTP configuration", __func__);

    memset(&ttp_gen_enable, 0x0, sizeof(struct afe_ttp_gen_enable));

    /* disable TTP generator */
    ttp_gen_enable.enable = 0x00;

    enable_ttp = mixer_get_ctl_by_name(a2dp_sink.adev->mixer, MIXER_TTP_GEN_ENABLE);
    if (!enable_ttp) {
        ALOGE("%s: ERROR ttp enable mixer control not identified", __func__);
        return;
    }

    ret = mixer_ctl_set_array(enable_ttp, (void *)&ttp_gen_enable,
                              sizeof(struct afe_ttp_gen_enable));
    if (ret != 0) {
        ALOGE("%s: Failed to disable ttp generator", __func__);
    }
    return;
}

static int enable_ttp_generator()
{
    struct afe_ttp_gen_enable ttp_gen_enable;
    struct afe_ttp_gen_cfg_num_offset ttp_gen_cfg_num_offset;
    struct mixer_ctl *enable_ttp = NULL, *set_ttp_config = NULL;

    ALOGV("%s: setting TTP configuration", __func__);

    memset(&ttp_gen_enable, 0x0, sizeof(struct afe_ttp_gen_enable));
    memset(&ttp_gen_cfg_num_offset, 0x0, sizeof(struct afe_ttp_gen_cfg_num_offset));

    /* enable TTP generator*/
    ttp_gen_enable.enable = ENABLE_TTP_GENERATOR;

    enable_ttp = mixer_get_ctl_by_name(a2dp_sink.adev->mixer, MIXER_TTP_GEN_ENABLE);
    if (!enable_ttp) {
        ALOGE("%s: ERROR ttp enable mixer control not identified", __func__);
        return -EINVAL;
    }

    if (mixer_ctl_set_array(enable_ttp, (void *)&ttp_gen_enable,
                              sizeof(struct afe_ttp_gen_enable))) {
        ALOGE("%s: Failed to enable ttp generator", __func__);
        return -EINVAL;
    }

    /* configure TTP generator parameters */
    ttp_gen_cfg_num_offset.ttp_offset_default = 0x00;
    ttp_gen_cfg_num_offset.settling_time = TTP_GEN_CONFIG_PARAM;
    ttp_gen_cfg_num_offset.settling_mode = 0x00;
    ttp_gen_cfg_num_offset.num_ssrc_offsets = MAX_TTP_OFFSET_PAIRS;
    memcpy(ttp_gen_cfg_num_offset.ssrc_ttp_offset, ssrc_ttp,
           MAX_TTP_OFFSET_PAIRS * sizeof(struct afe_ttp_ssrc_offset_pair));

    set_ttp_config = mixer_get_ctl_by_name(a2dp_sink.adev->mixer, MIXER_TTP_GEN_CONFIG);
    if (!set_ttp_config) {
        ALOGE("%s: ERROR ttp config mixer control not identified", __func__);
        goto err;
    }

    if (mixer_ctl_set_array(set_ttp_config, (void *)&ttp_gen_cfg_num_offset,
                              sizeof(struct afe_ttp_gen_cfg_num_offset))) {
        ALOGE("%s: Failed to set ttp configuration", __func__);
        goto err;
    }

    return 0;

err:
    disable_ttp_generator();
    return -EINVAL;
}

int audio_extn_a2dp_start_capture()
{
    int ret = 0;
    struct audio_usecase *uc_info;
    struct listnode *node;
    bool use_ttp_generator = true;

    ALOGD("audio_extn_a2dp_start_capture start");

    if (!(a2dp_sink.bt_lib_sink_handle && a2dp_sink.audio_sink_start
       && a2dp_sink.audio_get_dec_config && a2dp_sink.audio_sink_stop)) {
        ALOGE("a2dp handle is not identified, Ignoring start capture request");
        return -ENOSYS;
    }

    /*
     * Enable TTP generator for capture/loopback usecase
     * with AUDIO_DEVICE_IN_BLUETOOTH_A2DP as device and
     * with AUDIO_INPUT_FLAG_TIMESTAMP flags
     */
    list_for_each(node, &a2dp_sink.adev->usecase_list) {
        uc_info = node_to_item(node, struct audio_usecase, list);
        if ((uc_info->type == PCM_CAPTURE) &&
            (uc_info->devices & AUDIO_DEVICE_IN_BLUETOOTH_A2DP) &&
            (uc_info->stream.in->flags & AUDIO_INPUT_FLAG_TIMESTAMP)){
            a2dp_sink.enable_ttp = true;
        }
    }

    if (!a2dp_sink.a2dp_sink_started && !a2dp_sink.a2dp_sink_total_active_session_requests) {
        ALOGD("calling BT module stream start");
        /* This call indicates BT IPC lib to start capture */
        ret =  a2dp_sink.audio_sink_start();
        ALOGE("BT controller start capture return = %d",ret);
        if (ret != 0 ) {
           ALOGE("BT controller start capture failed");
           goto fail;
        } else {
           if (!audio_extn_a2dp_sink_is_ready()) {
                ALOGE("Wait for capture ready not successful");
                ret = -ETIMEDOUT;
                goto fail;
           }

           if (configure_a2dp_sink_decoder_format(&use_ttp_generator) == true) {
                a2dp_sink.a2dp_sink_started = true;
                ret = 0;
                ALOGD("Start capture successful to BT library");
           } else {
                ALOGE(" unable to configure DSP decoder");
                ret = -ETIMEDOUT;
                goto fail;
           }

           if (a2dp_sink.enable_ttp && use_ttp_generator) {
                ret = enable_ttp_generator();
                if (ret < 0) {
                    ALOGE("Failed to enable TTP generator");
                    a2dp_sink.enable_ttp = false;
                }
           } else if (!use_ttp_generator) {
                ALOGD("TTP generator not enabled");
           }

           if (!a2dp_send_sink_setup_complete()) {
               ALOGE("sink_setup_complete not successful");
               ret = -ETIMEDOUT;
               goto fail;
           }
        }
    }

    if (a2dp_sink.a2dp_sink_started) {
        if (a2dp_set_backend_cfg() == true) {
            a2dp_sink.a2dp_sink_total_active_session_requests++;
        }
    }

    ALOGD("start A2DP sink total active sessions :%d",
          a2dp_sink.a2dp_sink_total_active_session_requests);
    return ret;

fail:
    if (!a2dp_sink.a2dp_sink_total_active_session_requests)
        a2dp_sink.a2dp_sink_started = false;
    a2dp_sink.audio_sink_stop();
    return ret;
}

static void reset_a2dp_sink_dec_config_params()
{
    int ret =0;

    struct mixer_ctl *ctl_dec_config, *ctrl_bit_format;
    struct aac_dec_cfg_t dummy_reset_config;

    memset(&dummy_reset_config, 0x0, sizeof(struct aac_dec_cfg_t));
    ctl_dec_config = mixer_get_ctl_by_name(a2dp_sink.adev->mixer,
                                           MIXER_SINK_DEC_CONFIG_BLOCK);
    if (!ctl_dec_config) {
        ALOGE(" ERROR  a2dp decoder format mixer control not identified");
    } else {
        ret = mixer_ctl_set_array(ctl_dec_config, (void *)&dummy_reset_config,
                                        sizeof(struct aac_dec_cfg_t));
         a2dp_sink.bt_decoder_format = MEDIA_FMT_NONE;
    }
    ctrl_bit_format = mixer_get_ctl_by_name(a2dp_sink.adev->mixer,
                                            MIXER_DEC_BIT_FORMAT);
    if (!ctrl_bit_format) {
        ALOGE(" ERROR  bit format CONFIG data mixer control not identified");
    } else {
        ret = mixer_ctl_set_enum_by_string(ctrl_bit_format, "S16_LE");
        if (ret != 0) {
            ALOGE("%s: Failed to set bit format to decoder", __func__);
        }
    }
}

int audio_extn_a2dp_stop_capture()
{
    int ret =0;

    ALOGV("audio_extn_a2dp_stop_capture start");
    if (!(a2dp_sink.bt_lib_sink_handle && a2dp_sink.audio_sink_stop)) {
        ALOGE("a2dp handle is not identified, Ignoring stop request");
        return -ENOSYS;
    }

    if (a2dp_sink.a2dp_sink_total_active_session_requests > 0)
        a2dp_sink.a2dp_sink_total_active_session_requests--;

    if ( a2dp_sink.a2dp_sink_started && !a2dp_sink.a2dp_sink_total_active_session_requests) {
        ALOGV("calling BT module stream stop");
        ret = a2dp_sink.audio_sink_stop();
        if (ret < 0)
            ALOGE("stop stream to BT IPC lib failed");
        else
            ALOGV("stop steam to BT IPC lib successful");
        reset_a2dp_sink_dec_config_params();
        a2dp_reset_backend_cfg();
        if (a2dp_sink.enable_ttp) {
            disable_ttp_generator();
            a2dp_sink.enable_ttp = false;
        }
    }
    if (!a2dp_sink.a2dp_sink_total_active_session_requests)
       a2dp_sink.a2dp_sink_started = false;
    ALOGD("Stop A2DP capture, total active sessions :%d",
          a2dp_sink.a2dp_sink_total_active_session_requests);
    return 0;
}

void audio_extn_a2dp_sink_set_parameters(struct str_parms *parms)
{
    int ret, val;
    char value[32]={0};
    struct audio_usecase *uc_info;
    struct listnode *node;

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_DEVICE_DISCONNECT, value,
                         sizeof(value));

    if (ret >= 0) {
        val = atoi(value);
        if (audio_is_a2dp_in_device(val)) {
            ALOGV("Received sink device dis- connect request");
            close_a2dp_input();
            reset_a2dp_sink_dec_config_params();
            a2dp_reset_backend_cfg();
        }
    }

    ALOGV("end of a2dp setparam");
}

void audio_extn_a2dp_get_dec_sample_rate(int *sample_rate)
{
    *sample_rate = a2dp_sink.dec_sampling_rate;
}

bool audio_extn_a2dp_sink_is_ready()
{
    bool ret = false;

    if (a2dp_sink.audio_sink_check_a2dp_ready)
           ret = a2dp_sink.audio_sink_check_a2dp_ready();
    return ret;
}

void audio_extn_a2dp_sink_init (void *adev)
{
  a2dp_sink.adev = (struct audio_device*)adev;
  a2dp_sink.bt_lib_sink_handle = NULL;
  a2dp_sink.a2dp_sink_started = false;
  a2dp_sink.a2dp_sink_total_active_session_requests = 0;
  a2dp_sink.enable_ttp = false;
  reset_a2dp_sink_dec_config_params();
  open_a2dp_sink();
}
