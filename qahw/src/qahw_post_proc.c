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

#define LOG_TAG "qahw_post_proc"
//#define LOG_NDEBUG 0
#define LOG_NDDEBUG 0

#include <cutils/list.h>
#include <dlfcn.h>
#include <utils/Log.h>
#include <stdlib.h>
#include <pthread.h>

#include "qahw_post_proc.h"

// The current effect API version.
#define QAHW_POST_PROC_API_VERSION_CURRENT QAHW_POST_PROC_API_VERSION_0_0

static pthread_mutex_t module_lock = PTHREAD_MUTEX_INITIALIZER;

qahw_post_proc_module_handle_t *qahw_post_proc_load_module_l(const char *module_id) {
    qahw_post_proc_module_t *qahw_module;
    const char* error = NULL;

    pthread_mutex_lock(&module_lock);

    qahw_module = (qahw_post_proc_module_t *) malloc(sizeof(qahw_post_proc_module_t));

    qahw_module->lib_handle = dlopen(module_id, RTLD_NOW);
    if (qahw_module->lib_handle == NULL) {
        ALOGE("%s: failed to dlopen lib %s", __func__, module_id);
        return NULL;
    }

    dlerror();
    qahw_module->init =
        (adsp_post_proc_init_t)dlsym(qahw_module->lib_handle, "adsp_post_proc_init");
    error = dlerror();
    if(error != NULL) {
        ALOGE("%s: dlsym of %s failed with error %s",
             __func__, "adsp_post_proc_init", error);
        goto error;
    }

    dlerror();
    qahw_module->open_stream =
        (open_adsp_post_proc_stream_t)dlsym(qahw_module->lib_handle, "open_adsp_post_proc_stream");
    error = dlerror();
    if(error != NULL) {
        ALOGE("%s: dlsym of %s failed with error %s",
             __func__, "open_adsp_post_proc_stream", error);
        goto error;
    }

    dlerror();
    qahw_module->close_stream =
        (close_adsp_post_proc_stream_t)dlsym(qahw_module->lib_handle, "close_adsp_post_proc_stream");
    error = dlerror();
    if(error != NULL) {
        ALOGE("%s: dlsym of %s failed with error %s",
             __func__, "close_adsp_post_proc_stream", error);
        goto error;
    }

    dlerror();
    qahw_module->get_session_id =
        (get_adsp_post_proc_session_id_t)dlsym(qahw_module->lib_handle, "get_adsp_post_proc_session_id");
    error = dlerror();
    if(error != NULL) {
        ALOGE("%s: dlsym of %s failed with error %s",
             __func__, "get_adsp_post_proc_session_id", error);
        goto error;
    }

    dlerror();
    qahw_module->process =
        (adsp_process_t)dlsym(qahw_module->lib_handle, "adsp_process");
    error = dlerror();
    if(error != NULL) {
        ALOGE("%s: dlsym of %s failed with error %s",
             __func__, "adsp_process", error);
        goto error;
    }

    qahw_module->init();

    pthread_mutex_unlock(&module_lock);
    return (qahw_post_proc_module_handle_t *) qahw_module;

error:
    if (qahw_module->lib_handle) dlclose(qahw_module->lib_handle);
    free(qahw_module);

    pthread_mutex_unlock(&module_lock);
    return NULL;
}

int32_t qahw_open_post_proc_stream_l(qahw_post_proc_module_handle_t *module_handle,
                                uint32_t topology_id, uint32_t app_type,
                                qahw_post_proc_buffer_config_t *in_buf_config,
                                qahw_post_proc_buffer_config_t *out_buf_config,
                                qahw_post_proc_stream_handle_t **stream_handle) {
    int32_t rc = -EINVAL;
    qahw_post_proc_module_t *qahw_module = (qahw_post_proc_module_t *)module_handle;
    qahw_post_proc_stream_t *qahw_stream = (qahw_post_proc_stream_t *)
                                            malloc(sizeof(qahw_post_proc_stream_t));

    qahw_stream->qahw_module = qahw_module;
    qahw_stream->stream_handle = NULL;

    if (qahw_module) {
        rc = qahw_module->open_stream(topology_id, app_type,
                                            in_buf_config,
                                            out_buf_config,
                                            (void **) &qahw_stream->stream_handle);

        if (rc) {
            ALOGE("%s: failed to open stream", __func__);
            free(qahw_stream);
        } else
            *stream_handle = qahw_stream;
    }

    return rc;
}

int32_t qahw_close_post_proc_stream_l(qahw_post_proc_stream_handle_t *stream_handle) {
    int32_t rc = -EINVAL;
    qahw_post_proc_stream_t *qahw_stream = (qahw_post_proc_stream_t *) stream_handle;
    qahw_post_proc_module_t *qahw_module;

    if (qahw_stream) {
        qahw_module = qahw_stream->qahw_module;
        rc = qahw_module->close_stream(qahw_stream->stream_handle);
        free(qahw_stream);
    }

    return rc;
}

int qahw_post_proc_unload_module_l(qahw_post_proc_module_handle_t *module_handle) {
    qahw_post_proc_module_t *qahw_module = (qahw_post_proc_module_t *)module_handle;

    pthread_mutex_lock(&module_lock);

    if (qahw_module->lib_handle) dlclose(qahw_module->lib_handle);
    free(qahw_module);

    pthread_mutex_unlock(&module_lock);
    return 0;
}

int32_t qahw_get_post_proc_session_id_l(qahw_post_proc_stream_handle_t *stream_handle) {
    int32_t rc = -EINVAL;
    qahw_post_proc_stream_t *qahw_stream = (qahw_post_proc_stream_t *) stream_handle;
    qahw_post_proc_module_t *qahw_module;

    if (qahw_stream) {
        qahw_module = qahw_stream->qahw_module;
        rc = qahw_module->get_session_id(qahw_stream->stream_handle);
    }

    return rc;
}

int32_t qahw_process_l(qahw_post_proc_stream_handle_t *stream_handle,
                        qahw_post_proc_buffer_t *in_buf, qahw_post_proc_buffer_t *out_buf) {
    int32_t rc = -EINVAL;
    qahw_post_proc_stream_t *qahw_stream = (qahw_post_proc_stream_t *) stream_handle;
    qahw_post_proc_module_t *qahw_module;

    if (qahw_stream) {
        qahw_module = qahw_stream->qahw_module;
        rc = qahw_module->process(qahw_stream->stream_handle, in_buf, out_buf);
    }

    return rc;
}
