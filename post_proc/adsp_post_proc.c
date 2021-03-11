/*
 * Copyright (c) 2021, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
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

 #define LOG_TAG "adsp_post_proc"
/*#define LOG_NDEBUG 0*/

#include <system/audio.h>
#include <cutils/list.h>
#include <cutils/log.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <errno.h>

#include "adsp_post_proc.h"

#ifdef AUDIO_FEATURE_ENABLED_GCOV
extern void  __gcov_flush();
static void enable_gcov()
{
    __gcov_flush();
}
#else
static void enable_gcov()
{
}
#endif

typedef void (*audio_extn_adsp_post_proc_set_cal_t)(void*, int);

typedef struct {
    void *hal_data;

    void *hal_lib_handle;
    audio_extn_adsp_post_proc_set_cal_t set_cal;
} adsp_pp_private_data_t;

static adsp_pp_private_data_t priv_data;

__attribute__ ((visibility ("default")))
void adsp_post_proc_set_hal_data (void *data) {
    memset(&priv_data, 0, sizeof(adsp_pp_private_data_t));
    priv_data.hal_data = data;
}

__attribute__ ((visibility ("default")))
int adsp_post_proc_init () {
    char hal_lib_name[256];
    int ret;
    const char *error;

    if (priv_data.hal_data == NULL) {
        ALOGE("%s: hal_data is not initialized", __func__);
        return -EINVAL;
    }

    snprintf(hal_lib_name, sizeof(hal_lib_name),
        "/usr/lib/audio.primary.default.so");

    ret = access(hal_lib_name, R_OK);
    if(ret) {
        ALOGE("opening %s failed with error %s", hal_lib_name, strerror(ret));
        return ret;
    }
    priv_data.hal_lib_handle = dlopen(hal_lib_name, RTLD_LAZY);
    if (priv_data.hal_lib_handle == NULL) {
        ALOGE("%s: DLOPEN failed for %s", __func__, hal_lib_name);
        return -EINVAL;
    }

    error = dlerror();
    priv_data.set_cal = (audio_extn_adsp_post_proc_set_cal_t)
                        dlsym(priv_data.hal_lib_handle, "audio_extn_adsp_post_proc_set_cal");

    error = dlerror();
    if (error != NULL) {
      ALOGE("%s: dlsym failed for %s with error %s", __func__,
          "audio_extn_adsp_post_proc_set_cal", error);
      dlclose(priv_data.hal_lib_handle);
      priv_data.hal_lib_handle = NULL;
      return -EINVAL;
    }

    return ret;
}

__attribute__ ((visibility ("default")))
void adsp_post_proc_reset_hal_data () {
    priv_data.hal_data = NULL;
}

__attribute__ ((visibility ("default")))
void adsp_post_proc_deinit () {
    if (priv_data.hal_lib_handle)
        dlclose(priv_data.hal_lib_handle);
}

__attribute__ ((visibility ("default")))
int open_adsp_post_proc_stream (uint32_t topology_id, uint32_t app_type,
                                adsp_post_proc_buffer_config_t *in_buf_config, adsp_post_proc_buffer_config_t *out_buf_config,
                                adsp_post_proc_stream_handle_t **stream)
{
    adsp_post_proc_stream_t *adsp_stream = NULL;

    ALOGV("%s: topology_id %u, app_type %u", __func__, topology_id, app_type);

    adsp_stream = (adsp_post_proc_stream_t *)calloc(1, sizeof(adsp_post_proc_stream_t));

    if (priv_data.hal_data && priv_data.set_cal) {
        priv_data.set_cal(priv_data.hal_data, app_type);
        adsp_post_proc_set_config(adsp_stream, false, 0, app_type, in_buf_config, out_buf_config);
    } else {
        adsp_post_proc_set_config(adsp_stream, true, topology_id, app_type,
                                    in_buf_config, out_buf_config);
    }

    adsp_stream->fd = open("/dev/msm_hweffects", O_RDWR | O_NONBLOCK);
    /* open driver */
    if (adsp_stream->fd < 0) {
         ALOGE("driver open failed");
         return -EFAULT;
    }

    /* set config */
    if (ioctl(adsp_stream->fd, AUDIO_SET_EFFECTS_CONFIG_V2, &adsp_stream->config) < 0) {
        ALOGE("setting config failed");
        if (close(adsp_stream->fd) < 0)
            ALOGE("releasing driver failed");
        adsp_stream->fd = -1;
        return -EFAULT;
    }

    /* start */
    if (ioctl(adsp_stream->fd,  AUDIO_START, 0) < 0) {
        ALOGE("driver prepare failed");
        if (close(adsp_stream->fd) < 0)
            ALOGE("releasing driver failed");
        adsp_stream->fd = -1;
        return -EFAULT;
    }

    /* get session_id */
    if (ioctl(adsp_stream->fd, AUDIO_GET_SESSION_ID, &adsp_stream->session_id) < 0) {
        ALOGE("getting session_id failed");
        if (close(adsp_stream->fd) < 0)
            ALOGE("releasing driver failed");
        adsp_stream->fd = -1;
        return -EFAULT;
    }

    enable_gcov();

    *stream = adsp_stream;

    return 0;
}

int adsp_post_proc_set_config (adsp_post_proc_stream_t *adsp_stream,
                                bool overwrite_topology,
                                uint32_t topology_id, uint32_t app_type,
                                adsp_post_proc_buffer_config_t *in_buf_config,
                                adsp_post_proc_buffer_config_t *out_buf_config)
{
    adsp_stream->config.output.sample_rate = in_buf_config->sampling_rate;
    adsp_stream->config.input.sample_rate = out_buf_config->sampling_rate;

    adsp_stream->config.output.num_channels = in_buf_config->channels;
    adsp_stream->config.input.num_channels = out_buf_config->channels;

    adsp_stream->config.output.bits_per_sample = 8 *
                                audio_bytes_per_sample(in_buf_config->format);
    adsp_stream->config.input.bits_per_sample = 8 *
                                audio_bytes_per_sample(out_buf_config->format);

    ALOGV("write: sample_rate: %d, channel: %d, bit_width: %d",
           adsp_stream->config.output.sample_rate, adsp_stream->config.output.num_channels,
           adsp_stream->config.output.bits_per_sample);
    ALOGV("read: sample_rate: %d, channel: %d, bit_width: %d",
           adsp_stream->config.input.sample_rate, adsp_stream->config.input.num_channels,
           adsp_stream->config.input.bits_per_sample);

    adsp_stream->config.output.num_buf = 1;
    adsp_stream->config.input.num_buf = 1;

    adsp_stream->config.output.buf_size = in_buf_config->max_frame_count *
                    adsp_stream->config.input.num_channels *
                    audio_bytes_per_sample(in_buf_config->format);

    adsp_stream->config.input.buf_size = out_buf_config->max_frame_count *
                    adsp_stream->config.output.num_channels *
                    audio_bytes_per_sample(out_buf_config->format);

    adsp_stream->config.meta_mode_enabled = 0;

    adsp_stream->config.overwrite_topology = overwrite_topology;
    adsp_stream->config.topology = topology_id;

    return 0;
}

__attribute__ ((visibility ("default")))
int32_t close_adsp_post_proc_stream (adsp_post_proc_stream_handle_t *stream) {
    adsp_post_proc_stream_t *adsp_stream = (adsp_post_proc_stream_t *) stream;

    ALOGV("%s: stream %p", __func__, stream);

    if (adsp_stream->fd > 0)
        if (close(adsp_stream->fd) < 0)
            ALOGE("releasing hardware accelerated effects driver failed");
    adsp_stream->fd = -1;

    free(adsp_stream);

    enable_gcov();
    return 0;
}

__attribute__ ((visibility ("default")))
int get_adsp_post_proc_session_id (adsp_post_proc_stream_handle_t *stream) {
    adsp_post_proc_stream_t *adsp_stream = (adsp_post_proc_stream_t *) stream;

    ALOGV("%s: stream %p", __func__, stream);

    return adsp_stream->session_id;
}

__attribute__ ((visibility ("default")))
int adsp_process(adsp_post_proc_stream_handle_t *stream, adsp_post_proc_buffer_t *in_buf,
                    adsp_post_proc_buffer_t *out_buf) {
    struct msm_hwacc_buf_cfg buf_config;
    struct msm_hwacc_buf_avail buf_avail;
    int ret = 0;

    adsp_post_proc_stream_t *adsp_stream = (adsp_post_proc_stream_t *) stream;

    //ALOGV("%s: stream %p, input frame count %u", __func__, stream, in_buf->frame_count);

    if (in_buf == NULL || in_buf->raw == NULL || out_buf == NULL || out_buf->raw == NULL)
        return -EINVAL;

    buf_config.output_len = in_buf->frame_count *
                         (adsp_stream->config.input.bits_per_sample / 8) *
                         adsp_stream->config.input.num_channels;


    buf_config.input_len = out_buf->frame_count *
                         (adsp_stream->config.output.bits_per_sample / 8) *
                         adsp_stream->config.output.num_channels;

    if (ioctl(adsp_stream->fd, AUDIO_EFFECTS_GET_BUF_AVAIL, &buf_avail) < 0) {
        ALOGE("AUDIO_EFFECTS_GET_BUF_AVAIL failed");
        return -ENOMEM;
    }

    if (buf_avail.output_num_avail > 0) {
        if (ioctl(adsp_stream->fd, AUDIO_EFFECTS_SET_BUF_LEN, &buf_config) < 0) {
            ALOGE("AUDIO_EFFECTS_config failed");
            return -EFAULT;
        }
        if (ioctl(adsp_stream->fd, AUDIO_EFFECTS_WRITE, (char *)in_buf->raw) < 0) {
            ALOGE("AUDIO_EFFECTS_WRITE failed");
            return -EFAULT;
        }
        ret = in_buf->frame_count;
    }

    if (ioctl(adsp_stream->fd, AUDIO_EFFECTS_READ, (char *)out_buf->raw) < 0) {
        ALOGE("AUDIO_EFFECTS_READ failed");
        return -EFAULT;
    }

    return ret;
}
