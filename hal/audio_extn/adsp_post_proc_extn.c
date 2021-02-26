/*
 * Copyright (c) 2021, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of The Linux Foundation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
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

#define LOG_TAG "adsp_post_proc_extn"

#define LOG_NDEBUG 0
#define LOG_NDDEBUG 0

#include <errno.h>
#include <math.h>
#include <cutils/log.h>
#include <fcntl.h>
#include <dirent.h>
#include "audio_hw.h"
#include "platform.h"
#include "platform_api.h"
#include <sys/stat.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <math.h>
#include <cutils/properties.h>
#include "audio_extn.h"
#include "audio_hw.h"

#define ADSP_POST_PROC_LIB "libadsppostproc.so"

typedef void (*adsp_post_proc_set_hal_data_t)(void*);
typedef void (*adsp_post_proc_reset_hal_data_t)();

typedef struct {
    void* lib_handle;
    adsp_post_proc_set_hal_data_t set_hal_data;
    adsp_post_proc_reset_hal_data_t reset_hal_data;
} adsp_pp_hal_private_data_t;

static adsp_pp_hal_private_data_t priv_data;

void audio_extn_adsp_post_proc_init(struct audio_device *adev)
{
    const char* error = NULL;

    ALOGV("%s: Enter", __func__);

    memset(&priv_data, 0, sizeof(adsp_pp_hal_private_data_t));

    //: check error for dlopen
    priv_data.lib_handle = dlopen(ADSP_POST_PROC_LIB, RTLD_LAZY);
    if (priv_data.lib_handle == NULL) {
        ALOGE("%s: DLOPEN failed for %s with error %s",
            __func__, ADSP_POST_PROC_LIB, dlerror());
        goto ERROR_RETURN;
    } else {
        ALOGV("%s: DLOPEN successful for %s", __func__, ADSP_POST_PROC_LIB);

        dlerror();
        priv_data.set_hal_data =
            (adsp_post_proc_set_hal_data_t)dlsym(priv_data.lib_handle, "adsp_post_proc_set_hal_data");

        error = dlerror();
        if(error != NULL) {
            ALOGE("%s: dlsym of %s failed with error %s",
                 __func__, "adsp_post_proc_set_hal_data", error);
            goto ERROR_RETURN;
        }

        dlerror();
        priv_data.reset_hal_data =
            (adsp_post_proc_reset_hal_data_t)dlsym(priv_data.lib_handle, "adsp_post_proc_reset_hal_data");

        error = dlerror();
        if(error != NULL) {
            ALOGE("%s: dlsym of %s failed with error %s",
                 __func__, "adsp_post_proc_reset_hal_data", error);
            goto ERROR_RETURN;
        }

        priv_data.set_hal_data((void*)adev);
        return;
    }

ERROR_RETURN:
    ALOGV("%s: Exit with error ", __func__);
    return;
}

__attribute__ ((visibility ("default")))
void audio_extn_adsp_post_proc_set_cal (void *dev, int app_type) {
    struct audio_device *adev = (struct audio_device*)dev;
    int ret;

    pthread_mutex_lock(&adev->lock);
    ret = platform_send_non_tunnel_asm_calibration(adev->platform, app_type);
    pthread_mutex_unlock(&adev->lock);
}

void audio_extn_adsp_post_proc_deinit() {
    if (priv_data.lib_handle) {
        if (priv_data.reset_hal_data)
            priv_data.reset_hal_data();
        dlclose(priv_data.lib_handle);
    }
}