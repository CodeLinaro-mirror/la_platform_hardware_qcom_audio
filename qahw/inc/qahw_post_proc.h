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

#ifndef QTI_AUDIO_QAHW_POST_PROC_H
#define QTI_AUDIO_QAHW_POST_PROC_H

#include <errno.h>
#include <stdint.h>
#include <strings.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/time.h>
#include <cutils/bitops.h>

#include "qahw_post_proc_defs.h"

__BEGIN_DECLS

#define QAHW_POST_PROC_API_VERSION_0_0 QAHW_MAKE_API_VERSION(0, 0)
#define QAHW_POST_PROC_API_VERSION_MIN QAHW_POST_API_VERSION_0_0

typedef int (*adsp_post_proc_init_t)();
typedef int (*open_adsp_post_proc_stream_t)(uint32_t, uint32_t, qahw_post_proc_buffer_config_t*,
                qahw_post_proc_buffer_config_t*, void**);
typedef int (*close_adsp_post_proc_stream_t)(void*);
typedef int (*get_adsp_post_proc_session_id_t)(void*);
typedef int (*adsp_process_t)(void*, qahw_post_proc_buffer_t*, qahw_post_proc_buffer_t*);

typedef void qahw_post_proc_module_handle_t;
typedef void qahw_post_proc_stream_handle_t;

typedef struct {
    void *lib_handle;
    adsp_post_proc_init_t init;
    open_adsp_post_proc_stream_t open_stream;
    close_adsp_post_proc_stream_t close_stream;
    get_adsp_post_proc_session_id_t get_session_id;
    adsp_process_t process;
} qahw_post_proc_module_t;

typedef struct {
    qahw_post_proc_module_t *qahw_module;
    qahw_post_proc_stream_handle_t *stream_handle;
} qahw_post_proc_stream_t;

#ifdef __cplusplus
extern "C"
{
#endif

qahw_post_proc_module_handle_t *qahw_post_proc_load_module_l(const char *module_id);

int qahw_post_proc_unload_module_l(qahw_post_proc_module_handle_t *module_handle);

int32_t qahw_open_post_proc_stream_l(qahw_post_proc_module_handle_t *module_handle,
                                        uint32_t topology_id, uint32_t app_type,
                                        qahw_post_proc_buffer_config_t *in_buf_config,
                                        qahw_post_proc_buffer_config_t *out_buf_config,
                                        qahw_post_proc_stream_handle_t **stream_handle);

int32_t qahw_close_post_proc_stream_l(qahw_post_proc_stream_handle_t *stream_handle);

int32_t qahw_get_post_proc_session_id_l(qahw_post_proc_stream_handle_t *stream_handle);

int32_t qahw_process_l(qahw_post_proc_stream_handle_t *stream_handle,
                    qahw_post_proc_buffer_t *in_buf, qahw_post_proc_buffer_t *out_buf);

#ifdef __cplusplus
}
#endif

__END_DECLS

#endif  // QTI_AUDIO_QAHW_POST_PROC_H
