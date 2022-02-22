/*
* Copyright (c) 2016-2021, The Linux Foundation. All rights reserved.
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
*
* Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted (subject to the limitations in the disclaimer
* below) provided that the following conditions are met:
*
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of Qualcomm Innovation Center, Inc. nor the names
*       of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED
* BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
* CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
* BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
* FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
* HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
* TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
* PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
* EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#define LOG_TAG "audio_hw_cin"
/*#define LOG_NDEBUG 0*/
#define LOG_NDDEBUG 0

#ifdef COMPRESS_INPUT_ENABLED
#include <inttypes.h>
#include <cutils/log.h>
#include <cutils/properties.h>
#include <cutils/str_parms.h>
#include <cutils/atomic.h>
#include <log/log.h>
#include <pthread.h>
#include <unistd.h>

#include "audio_hw.h"
#include "platform.h"
#include "platform_api.h"

#include <hardware/audio.h>
#include <errno.h>
#include <time.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <signal.h>

#include "audio_extn.h"
#include "audio_defs.h"
#include "sound/compress_params.h"
#include <sound/compress_offload.h>

#ifdef DYNAMIC_LOG_ENABLED
#include <log_xml_parser.h>
#define LOG_MASK HAL_MOD_FILE_COMPR_IN
#include <log_utils.h>
#endif
/* default timestamp metadata definition if not defined in kernel*/
#ifndef COMPRESSED_TIMESTAMP_FLAG
#define COMPRESSED_TIMESTAMP_FLAG 0
struct snd_codec_metadata {
uint64_t timestamp;
};
#define compress_config_set_timstamp_flag(config) (-ENOSYS)
#else
#define compress_config_set_timstamp_flag(config) \
            (config)->codec->flags |= COMPRESSED_TIMESTAMP_FLAG
#endif

#define COMPRESS_RECORD_NUM_FRAGMENTS 8

#define CIN_STOP_WAIT_TIMEOUT_SEC   0
#define CIN_STOP_WAIT_TIMEOUT_MSEC  2
#define CIN_STOP_WAIT_TIMEOUT_USEC  (CIN_STOP_WAIT_TIMEOUT_MSEC * 1000)
#define CIN_STOP_WAIT_TIMEOUT_NSEC  (CIN_STOP_WAIT_TIMEOUT_MSEC * 1000000)
#define BYTES_PER_SAMPLE_16BIT      2
#define NUM_TIMEOUT_BUF             10

#define GET_WAIT_TIMESPEC(timeout, t_sec, t_nsec) \
do {\
    clock_gettime(CLOCK_REALTIME, &timeout); \
    timeout.tv_sec += t_sec; \
    timeout.tv_nsec += t_nsec; \
} while(0)

struct cin_private_data {
    struct compr_config compr_config;
    struct compress *compr;
    bool usecase_acquired;
    pthread_mutex_t cin_read_lock;
};

typedef struct cin_private_data cin_private_data_t;

static unsigned int cin_usecases_state;

static const audio_usecase_t cin_usecases[] = {
    USECASE_AUDIO_RECORD_COMPRESS2,
    USECASE_AUDIO_RECORD_COMPRESS3,
    USECASE_AUDIO_RECORD_COMPRESS4,
    USECASE_AUDIO_RECORD_COMPRESS5,
    USECASE_AUDIO_RECORD_COMPRESS6
};

static pthread_mutex_t cin_lock = PTHREAD_MUTEX_INITIALIZER;

static int cin_compress_in_set_ttp_metadata(
                  struct stream_in *in);

static void timeout_handler(int sig, siginfo_t *si, void *uc);
static int start_timer(timer_t timerid, long timeout);
static int stop_timer(timer_t timerid);
static int delete_timer(struct stream_in *in);
static int create_timer(struct stream_in *in);
static long calculate_timeout_ns(struct stream_in *in, size_t bytes);

static void timeout_handler(int sig, siginfo_t *si, void *uc)
{
    timer_t *gtimer;
    gtimer = si->si_value.sival_ptr;

    struct stream_in *in = (struct stream_in *)gtimer;
    cin_private_data_t *cin_data = (cin_private_data_t *) in->cin_extn;

    if (cin_data->compr) {
        ALOGV("%s: stop done, caught signal %d", __func__, sig);
        pthread_mutex_unlock(&cin_data->cin_read_lock);
        compress_stop(cin_data->compr);
        ALOGV("%s: compress stop done", __func__);
    } else
        ALOGD("%s: HDMI session is already closed", __func__);
}

static int start_timer(timer_t timerid, long timeout)
{
    struct itimerspec its;
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = timeout;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = timeout;

    if (timer_settime(timerid, 0, &its, NULL) == -1) {
        ALOGE("%s: timer_settime", __func__);
        return -1;
    }
    return 0;
}

static int stop_timer(timer_t timerid)
{
    struct itimerspec its;
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;

    if (timer_settime(timerid, 0, &its, NULL) == -1) {
        ALOGE("%s: timer_settime", __func__);
        return -1;
    }
    return 0;
}

static int delete_timer(struct stream_in *in)
{
    if (in->timer_handle) {
        timer_delete(in->timer_handle);
        in->timer_handle = 0;
    }
    in->calc_timeout = false;
    in->hdmi_in_wait_ns = 0;
    return 0;
}

static int create_timer(struct stream_in *in)
{
    struct sigevent sev;
    struct sigaction sa;

    /* Establish handler for timer signal */
    ALOGV(" %s: Establishing handler", __func__);
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = timeout_handler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGRTMIN, &sa, NULL) == -1) {
        ALOGE("%s: sigaction failed", __func__);
        return -1;
    }

    /* Create the timer */
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGRTMIN;
    sev.sigev_value.sival_ptr = (struct stream_in *)in;
    if (timer_create(CLOCK_REALTIME, &sev, &(in->timer_handle)) == -1) {
        ALOGE("%s: timer_create", __func__);
        return -1;
    }
    return 0;
}

static long calculate_timeout_ns(struct stream_in *in, size_t bytes)
{
    long timeout = 0;
    if (in->sample_rate != 0) {
        timeout = ((((double)bytes / in->config.channels) /
                   BYTES_PER_SAMPLE_16BIT) / (double)(in->sample_rate)) *
                   NUM_TIMEOUT_BUF * 1000000000;
    }
    return timeout;
}

bool cin_applicable_stream(struct stream_in *in)
{
    if (in->flags & (AUDIO_INPUT_FLAG_COMPRESS | AUDIO_INPUT_FLAG_TIMESTAMP))
        return true;

    return false;
}

/* all cin_xxx calls must be made on an input
 * only after validating that input against cin_attached_usecase
 * except below calls
 * 1. cin_applicable_stream(in)
 * 2. cin_configure_input_stream(in, in_config)
 */

bool cin_attached_usecase(struct stream_in *in)
{
    unsigned int i = 0;
    audio_usecase_t uc_id = in->usecase;

    for (i = 0; i < sizeof(cin_usecases)/
                    sizeof(cin_usecases[0]); i++) {
        if (uc_id == cin_usecases[i] && in->cin_extn != NULL)
            return true;
    }
    return false;
}

static audio_usecase_t get_cin_usecase(void)
{
    audio_usecase_t ret_uc = USECASE_INVALID;
    unsigned int i, num_usecase = sizeof(cin_usecases) / sizeof(cin_usecases[0]);
    char value[PROPERTY_VALUE_MAX] = {0};

    property_get("vendor.audio.record.multiple.enabled", value, NULL);
    if (!(atoi(value) || !strncmp("true", value, 4)))
        num_usecase = 1; /* If prop is not set, limit the num of record usecases to 1 */

    ALOGV("%s: num_usecase: %d", __func__, num_usecase);
    pthread_mutex_lock(&cin_lock);
    for (i = 0; i < num_usecase; i++) {
        if (!(cin_usecases_state & (0x1 << i))) {
            cin_usecases_state |= 0x1 << i;
            ret_uc = cin_usecases[i];
            break;
        }
    }
    pthread_mutex_unlock(&cin_lock);
    ALOGV("%s: picked usecase: %d", __func__, ret_uc);
    return ret_uc;
}

static void free_cin_usecase(audio_usecase_t uc_id)
{
    unsigned int i;

    ALOGV("%s: free usecase %d", __func__, uc_id);
    pthread_mutex_lock(&cin_lock);
    for (i = 0; i < sizeof(cin_usecases) /
                    sizeof(cin_usecases[0]); i++) {
        if (uc_id == cin_usecases[i]) {
            cin_usecases_state &= ~(0x1 << i);
            break;
        }
    }
    pthread_mutex_unlock(&cin_lock);
}

bool cin_format_supported(audio_format_t format)
{
    if ((format == AUDIO_FORMAT_IEC61937) || (format == AUDIO_FORMAT_DSD))
        return true;
    else
        return false;
}

int cin_acquire_usecase(struct stream_in *in)
{
    audio_usecase_t usecase = USECASE_INVALID;
    cin_private_data_t *cin_data = (cin_private_data_t *) in->cin_extn;

    if (cin_data->usecase_acquired) {
        ALOGW("%s: in %p, usecase already acquired!", __func__, in);
        return 0;
    }

    usecase = get_cin_usecase();
    if (usecase == USECASE_INVALID) {
        ALOGE("%s: in %p, failed to acquire usecase, max count reached!", __func__, in);
        return -EBUSY;
    }

    in->usecase = usecase;
    cin_data->usecase_acquired = true;
    return 0;
}

size_t cin_get_buffer_size(struct stream_in *in)
{
    size_t sz = 0;
    cin_private_data_t *cin_data = (cin_private_data_t *) in->cin_extn;

    sz = cin_data->compr_config.fragment_size;
    if ((in->flags & AUDIO_INPUT_FLAG_TIMESTAMP) ||
        (in->flags & AUDIO_INPUT_FLAG_PASSTHROUGH))
        sz -= sizeof(struct snd_codec_metadata);

    ALOGV("%s: in %p, flags 0x%x, cin_data %p, size %zd",
                  __func__, in, in->flags, cin_data, sz);
    return sz;
}

#ifdef SNDRV_COMPRESS_RENDER_MODE
static int cin_compress_set_render_mode(struct stream_in *in)
{
    struct snd_compr_metadata metadata;
    int ret = -EINVAL;
    cin_private_data_t *cin_data = (cin_private_data_t *) in->cin_extn;

    if (!cin_attached_usecase(in)) {
        ALOGE("%s:: not supported for non offload session", __func__);
        goto exit;
    }

    if (!cin_data->compr) {
        ALOGW("%s: offload session not yet opened", __func__);
       goto exit;
    }

    ALOGD("%s:: render mode %d", __func__, in->render_mode);

    metadata.key = SNDRV_COMPRESS_RENDER_MODE;
    if (in->render_mode == RENDER_MODE_AUDIO_MASTER) {
        metadata.value[0] = SNDRV_COMPRESS_RENDER_MODE_AUDIO_MASTER;
    } else if (in->render_mode == RENDER_MODE_AUDIO_STC_MASTER) {
        metadata.value[0] = SNDRV_COMPRESS_RENDER_MODE_STC_MASTER;
    } else if (in->render_mode == RENDER_MODE_AUDIO_TTP) {
        metadata.value[0] = SNDRV_COMPRESS_RENDER_MODE_TTP;
    } else {
        ret = 0;
        ALOGE("%s:: invalid render mode %d", __func__, in->render_mode);
        goto exit;
    }
    ret = compress_set_metadata(cin_data->compr, &metadata);
    if(ret) {
        ALOGE("%s::error %s", __func__, compress_get_error(cin_data->compr));
    }
exit:
    return ret;
}
#else
static int cin_compress_set_render_mode(struct stream_in *in __unused)
{
    ALOGD("%s:: configuring render mode not supported", __func__);
    return 0;
}
#endif

int cin_open_input_stream(struct stream_in *in)
{
    int ret = -EINVAL;
    struct audio_device *adev = in->dev;
    cin_private_data_t *cin_data = (cin_private_data_t *) in->cin_extn;

    ALOGV("%s: in %p, cin_data %p", __func__, in, cin_data);

    if (!cin_data->usecase_acquired) {
        ALOGE("%s: in %p, invalid state: usecase not acquired yet!", __func__, in);
        return ret;
    }

    cin_data->compr = compress_open(adev->snd_card, in->pcm_device_id,
                                    COMPRESS_OUT, &cin_data->compr_config);
    if (cin_data->compr == NULL || !is_compress_ready(cin_data->compr)) {
        ALOGE("%s: %s", __func__,
              cin_data->compr ? compress_get_error(cin_data->compr) : "null");
        if (cin_data->compr) {
            compress_close(cin_data->compr);
            cin_data->compr = NULL;
        }
        return -EIO;
    } else {
        ret = 0;
    }

    if ((true == in->hdmi_in_status) &&
        (!(in->flags & (AUDIO_INPUT_FLAG_TIMESTAMP)))) {
        ret = create_timer(in);
        if (0 != ret) {
            ALOGE("%s: Timer creation failed", __func__);
            return ret;
        }
        in->calc_timeout = true;
        in->hdmi_in_wait_ns = 0;
        ALOGD("%s:%d HDMI IN device connected", __func__, __LINE__);
    }

    if ((in->flags & AUDIO_INPUT_FLAG_TIMESTAMP) &&
        (in->render_mode == RENDER_MODE_AUDIO_TTP) &&
        (in->ttp_offset_cached)) {
        ALOGD("set ttp offset:0x%"PRIx64" ", in->ttp_offset_cached);
        cin_compress_in_set_ttp_metadata(in);
    }
    cin_compress_set_render_mode(in);

    return ret;
}

void cin_stop_input_stream(struct stream_in *in)
{
    int ret;
    struct timespec tspec;
    cin_private_data_t *cin_data = (cin_private_data_t *) in->cin_extn;

    ALOGD("%s: in %p, cin_data %p", __func__, in, cin_data);
    if (cin_data->compr) {
        /*Try to acquire read lock and call compress_stop*/
        ret = pthread_mutex_trylock(&cin_data->cin_read_lock);
        compress_stop(cin_data->compr);
        if (ret == 0) {
            /*if lock is acquired then unlock*/
            pthread_mutex_unlock(&cin_data->cin_read_lock);
            ALOGD("%s: stop done", __func__);
        } else {
            while (true) {
                /*if failed to get lock then wait to acquire lock*/
                GET_WAIT_TIMESPEC(tspec, CIN_STOP_WAIT_TIMEOUT_SEC, CIN_STOP_WAIT_TIMEOUT_NSEC);
                ALOGD("%s: wait for read completion", __func__);
                ret = pthread_mutex_timedlock(&cin_data->cin_read_lock, &tspec);
                if (ret == 0) {
                    /*if acquired lock then unlock and break*/
                    ALOGD("%s: stop done", __func__);
                    pthread_mutex_unlock(&cin_data->cin_read_lock);
                    break;
                } else if (ret == ETIMEDOUT) {
                    ALOGD("%s: stop wait timed out, calling stop", __func__);
                } else {
                    ALOGD("%s: unknown issue, force wait and call stop", __func__);
                    usleep(CIN_STOP_WAIT_TIMEOUT_USEC);
                }
                /*if acquired lock timed out then call compress_stop and wait again*/
                compress_stop(cin_data->compr);
            }
        }
    }
}

void cin_close_input_stream(struct stream_in *in)
{
    cin_private_data_t *cin_data = (cin_private_data_t *) in->cin_extn;

    ALOGV("%s: in %p, cin_data %p", __func__, in, cin_data);

    delete_timer(in);

    if (cin_data->compr) {
        compress_close(cin_data->compr);
        cin_data->compr = NULL;
    }

    if (cin_data->usecase_acquired) {
        free_cin_usecase(in->usecase);
        cin_data->usecase_acquired = false;
    }
}

void cin_free_input_stream_resources(struct stream_in *in)
{
    cin_private_data_t *cin_data = (cin_private_data_t *) in->cin_extn;

    ALOGV("%s: in %p, cin_data %p", __func__, in, cin_data);
    if (cin_data) {
        pthread_mutex_destroy(&cin_data->cin_read_lock);
        free(cin_data->compr_config.codec);
        free(cin_data);
    }
}

int cin_read(struct stream_in *in, void *buffer,
                        size_t bytes, size_t *bytes_read)
{
    int ret = -EINVAL;
    size_t read_size = bytes;
    size_t mdata_size = (sizeof(struct snd_codec_metadata));
    cin_private_data_t *cin_data = (cin_private_data_t *) in->cin_extn;

    if (true == in->calc_timeout) {
        in->hdmi_in_wait_ns = calculate_timeout_ns(in, bytes);
        in->calc_timeout = false;
        ALOGV("%s: hdmi_in_wait_ns = %ld", __func__, in->hdmi_in_wait_ns);
    }

    if (cin_data->compr) {
        pthread_mutex_lock(&cin_data->cin_read_lock);

        /* Avoid read if capture_stopped is set */
        if (android_atomic_acquire_load(&(in->capture_stopped)) > 0) {
            ALOGI("%s: force stopped catpure session, ignoring read request", __func__);
            pthread_mutex_unlock(&cin_data->cin_read_lock);
            goto exit;
        }

        /* start stream if not already done */
        if (!is_compress_running(cin_data->compr))
            compress_start(cin_data->compr);

        if (!(in->flags & (AUDIO_INPUT_FLAG_TIMESTAMP | AUDIO_INPUT_FLAG_PASSTHROUGH)))
            mdata_size = 0;

        if (buffer && read_size) {
            /* start timer to calculate 200ms timeout */
            if ((true == in->hdmi_in_status) &&
                (!(in->flags & (AUDIO_INPUT_FLAG_TIMESTAMP))))
                start_timer(in->timer_handle, in->hdmi_in_wait_ns);

            read_size = compress_read(cin_data->compr, buffer, read_size);
            /* stop timer in case of success return by compress_read */
            if ((true == in->hdmi_in_status) &&
                (!(in->flags & (AUDIO_INPUT_FLAG_TIMESTAMP))))
                stop_timer(in->timer_handle);

            pthread_mutex_unlock(&cin_data->cin_read_lock);
            if (read_size == bytes) {
                /* set ret to 0 if compress_read succeeded*/
                ret = 0;
                *bytes_read = bytes;
                /* data from DSP comes in 24_8 format, convert it to 8_24 */
                if (in->format == AUDIO_FORMAT_PCM_8_24_BIT) {
                    if (audio_extn_utils_convert_format_24_8_to_8_24(
                                          (char *)buffer + mdata_size, bytes) != bytes)
                        ret = -EIO;
                }
            } else {
                ret = errno;
                ALOGE("%s: failed error = %d, read = %zd, err_str %s", __func__,
                           ret, read_size, compress_get_error(cin_data->compr));
            }
        } else {
            pthread_mutex_unlock(&cin_data->cin_read_lock);
        }
    }

exit:
    ALOGV("%s: in %p, flags 0x%x, buf %p, bytes %zd, read_size %zd, ret %d",
                        __func__, in, in->flags, buffer, bytes, read_size, ret);
    return ret;
}

int cin_configure_input_stream(struct stream_in *in, struct audio_config *in_config)
{
    struct audio_config config = {.format = 0};
    int ret = 0, buffer_size = 0, meta_size = sizeof(struct snd_codec_metadata);
    cin_private_data_t *cin_data = NULL;

    if (!COMPRESSED_TIMESTAMP_FLAG &&
        (in->flags & (AUDIO_INPUT_FLAG_TIMESTAMP | AUDIO_INPUT_FLAG_PASSTHROUGH))) {
        ALOGE("%s: timestamp mode not supported!", __func__);
        return -EINVAL;
    }

    cin_data = (cin_private_data_t *) calloc(1, sizeof(cin_private_data_t));
    in->cin_extn = (void *)cin_data;
    if (!cin_data) {
        ALOGE("%s, allocation for private data failed!", __func__);
        return -ENOMEM;
    }

    cin_data->compr_config.codec = (struct snd_codec *)
                              calloc(1, sizeof(struct snd_codec));
    if (!cin_data->compr_config.codec) {
        ALOGE("%s, allocation for codec data failed!", __func__);
        ret = -ENOMEM;
        goto err_config;
    }

    config.sample_rate = in->sample_rate;
    config.channel_mask = in->channel_mask;
    config.format = in->format;
    in->config.channels = audio_channel_count_from_in_mask(in->channel_mask);
    buffer_size = audio_extn_utils_get_input_buffer_size(config.sample_rate, config.format,
                    in->config.channels, in_config->offload_info.duration_us / 1000, false);

    cin_data->compr_config.fragment_size = buffer_size;
    cin_data->compr_config.codec->id = get_snd_codec_id(in->format);
    cin_data->compr_config.fragments = COMPRESS_RECORD_NUM_FRAGMENTS;
    cin_data->compr_config.codec->sample_rate = in->sample_rate;
    cin_data->compr_config.codec->ch_in = in->config.channels;
    cin_data->compr_config.codec->ch_out = in->config.channels;
    cin_data->compr_config.codec->format = hal_format_to_alsa(in->format);

    if (cin_data->compr_config.codec->id == SND_AUDIOCODEC_PCM)
        cin_data->compr_config.codec->compr_passthr = LEGACY_PCM;
    else if (cin_data->compr_config.codec->id == SND_AUDIOCODEC_IEC61937)
        cin_data->compr_config.codec->compr_passthr = PASSTHROUGH_IEC61937;
    else if (cin_data->compr_config.codec->id == SND_AUDIOCODEC_DSD)
        cin_data->compr_config.codec->compr_passthr = PASSTHROUGH_DSD;
    else
        cin_data->compr_config.codec->compr_passthr = PASSTHROUGH_GEN;

    if (in->flags & AUDIO_INPUT_FLAG_FAST) {
        ALOGD("%s: Setting latency mode to true", __func__);
        cin_data->compr_config.codec->flags |= audio_extn_utils_get_perf_mode_flag();
    }

    if ((in->flags & AUDIO_INPUT_FLAG_TIMESTAMP) ||
        (in->flags & AUDIO_INPUT_FLAG_PASSTHROUGH)) {
        compress_config_set_timstamp_flag(&cin_data->compr_config);
        cin_data->compr_config.fragment_size += meta_size;
    }

    if ((in->flags & AUDIO_INPUT_FLAG_TIMESTAMP) &&
        (property_get_bool("persist.vendor.audio.ttp.render.mode", false)))
        in->render_mode = RENDER_MODE_AUDIO_TTP;
    else
        in->render_mode = RENDER_MODE_AUDIO_NO_TIMESTAMP;

    pthread_mutex_init(&cin_data->cin_read_lock, NULL);

    ALOGD("%s: format %d flags 0x%x SR %d CM 0x%x buf_size %d in %p",
          __func__, in->format, in->flags, in->sample_rate, in->channel_mask,
          cin_data->compr_config.fragment_size, in);
    return ret;

err_config:
    cin_free_input_stream_resources(in);
    return ret;
}

#ifdef SNDRV_COMPRESS_IN_TTP_OFFSET
int cin_compress_in_set_ttp_offset(
            struct stream_in *in,
            struct audio_in_ttp_offset_param *offset_param)
{
    int ret = -EINVAL;
    cin_private_data_t *cin_data = (cin_private_data_t *) in->cin_extn;

    if (offset_param == NULL) {
        ALOGE("%s: Invalid param", __func__);
        goto exit;
    }

    ALOGD("%s: ttp offset 0x%"PRIx64" ", __func__, offset_param->ttp_offset);

    if (!cin_attached_usecase(in)) {
        ALOGE("%s:: not supported for non offload session", __func__);
        goto exit;
    }

    in->ttp_offset_cached = offset_param->ttp_offset;

    if (!cin_data->compr) {
        ALOGW("%s: offload session not yet opened", __func__);
        goto exit;
    }

    ret = cin_compress_in_set_ttp_metadata(in);
    if (ret) {
        ALOGE("%s: error: set ttp offset metadata failed", __func__);
    }

exit:
    return ret;
}

static int cin_compress_in_set_ttp_metadata(struct stream_in *in)
{
    struct snd_compr_metadata metadata;
    int ret = -EINVAL;
    cin_private_data_t *cin_data = (cin_private_data_t *) in->cin_extn;

    if (!in) {
        ALOGE("%s: Invalid Param", __func__);
        return ret;
    }

    metadata.key = SNDRV_COMPRESS_IN_TTP_OFFSET;
    metadata.value[0] = 0xFFFFFFFF & in->ttp_offset_cached; /* LSB */
    metadata.value[1] = \
            (0xFFFFFFFF00000000 & in->ttp_offset_cached) >> 32; /* MSB*/

    ret = compress_set_metadata(cin_data->compr, &metadata);
    if (ret) {
        ALOGE("%s: error %s", __func__, compress_get_error(cin_data->compr));
    }

    return ret;
}
#else
int cin_compress_in_set_ttp_offset(
            struct stream_in *in __unused,
            struct audio_in_ttp_offset_param *offset_param __unused)
{
    ALOGD("%s: configuring ttp offset not supported", __func__);
    return 0;
}

static int cin_compress_in_set_ttp_metadata(
            struct stream_in *in)
{
    ALOGD("%s: configuring ttp offset metadata not supported", __func__);
    return 0;
}
#endif

#endif /* COMPRESS_INPUT_ENABLED end */
