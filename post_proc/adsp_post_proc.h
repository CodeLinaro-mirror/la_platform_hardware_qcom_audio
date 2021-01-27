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

#ifndef _QTI_ADSP_POST_PROC_H_
#define _QTI_ADSP_POST_PROC_H_

#include <linux/msm_audio.h>
#include <sys/cdefs.h>
#include <stdbool.h>

typedef void adsp_post_proc_stream_handle_t;

typedef struct {
    int fd;
    struct msm_hwacc_effects_config config;
    int session_id;
} adsp_post_proc_stream_t;

typedef struct {
    uint32_t   max_frame_count; // maximum possible frame count in a buffer
    uint32_t   sampling_rate;   // sampling rate
    uint32_t   channels;        // channel mask (see audio_channel_mask_t in audio.h)
    uint8_t    format;          // audio format (see audio_format_t in audio.h)
} adsp_post_proc_buffer_config_t;

typedef struct {
    size_t     frame_count;     // frame count
    void*      raw;             // void pointer to start of the buffer
} adsp_post_proc_buffer_t;

void adsp_post_proc_set_hal_data (void *data);
int adsp_post_proc_init ();
void adsp_post_proc_reset_hal_data ();
int open_adsp_post_proc_stream (uint32_t topology_id, uint32_t app_type,
                                adsp_post_proc_buffer_config_t *in_buf_config, adsp_post_proc_buffer_config_t *out_buf_config,
                                adsp_post_proc_stream_handle_t **stream);
int adsp_post_proc_set_config (adsp_post_proc_stream_t *adsp_stream, bool overwrite_topology,
                                uint32_t topology_id, uint32_t app_type,
                                adsp_post_proc_buffer_config_t *in_buf_config, adsp_post_proc_buffer_config_t *out_buf_config);
int32_t close_adsp_post_proc_stream (adsp_post_proc_stream_handle_t *stream);
int get_adsp_post_proc_session_id(adsp_post_proc_stream_handle_t *stream);
int adsp_process(adsp_post_proc_stream_handle_t *stream, adsp_post_proc_buffer_t *in_buf, adsp_post_proc_buffer_t *out_buf);

#endif
