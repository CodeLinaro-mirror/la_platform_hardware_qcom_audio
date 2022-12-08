/*
* Copyright (c) 2021, The Linux Foundation. All rights reserved.
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

#define LOG_TAG "qahw_post_proc_api"

#define LOG_NDEBUG 0
#define LOG_NDDEBUG 0

#include <utils/Errors.h>
#include <dlfcn.h>
#include <utils/Log.h>
#include <stdlib.h>
#include <cutils/list.h>
#include <assert.h>
#include <string.h>

#include <hardware/audio.h>
#include <cutils/properties.h>
#include <errno.h>

#include <qahw_post_proc_api.h>

#include "qahw_post_proc.h"

qahw_post_proc_module_handle_t *qahw_post_proc_load_module(const char *module_id)
{
    ALOGV("%d:%s",__LINE__, __func__);
    return qahw_post_proc_load_module_l(module_id);
}

int32_t qahw_open_post_proc_stream(qahw_post_proc_module_handle_t *module_handle,
                                        uint32_t topology_id, uint32_t app_type,
                                        qahw_post_proc_buffer_config_t *in_buf_config,
                                        qahw_post_proc_buffer_config_t *out_buf_config,
                                        qahw_post_proc_stream_handle_t **stream_handle)
{
    ALOGV("%d:%s",__LINE__, __func__);
    return qahw_open_post_proc_stream_l(module_handle, topology_id, app_type,
                                                    in_buf_config, out_buf_config, stream_handle);
}

int32_t qahw_close_post_proc_stream(qahw_post_proc_stream_handle_t *stream_handle)
{
    ALOGV("%d:%s",__LINE__, __func__);
    return qahw_close_post_proc_stream_l(stream_handle);
}

int qahw_post_proc_unload_module(qahw_post_proc_module_handle_t *module_handle)
{
    ALOGV("%d:%s",__LINE__, __func__);
    return qahw_post_proc_unload_module_l(module_handle);
}

int32_t qahw_get_post_proc_session_id(qahw_post_proc_stream_handle_t *stream_handle)
{
    ALOGV("%d:%s",__LINE__, __func__);
    return qahw_get_post_proc_session_id_l(stream_handle);
}

int32_t qahw_process(qahw_post_proc_stream_handle_t *stream_handle,
                    qahw_post_proc_buffer_t *in_buf, qahw_post_proc_buffer_t *out_buf)
{
    return qahw_process_l(stream_handle, in_buf, out_buf);
}
