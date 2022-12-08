/*
 * Copyright (c) 2019-2020, The Linux Foundation. All rights reserved.
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

//#define LOG_NDEBUG 0
#define LOG_TAG "ahal_config_helper"

#include <cutils/properties.h>
#include <dlfcn.h>
#include <log/log.h>
#include "ahal_config_helper.h"

struct AHalConfigHelper {
    static AHalConfigHelper* mConfigHelper;
    AHalConfigHelper() {};

    static AHalConfigHelper* getAHalConfInstance() {
        if (!mConfigHelper)
            mConfigHelper = new AHalConfigHelper();
        return mConfigHelper;
    }
    void initConfigHelper(bool isVendorEnhancedFwk);
    void getAHalValues(AHalValues* *confValues);
    AHalValues defaultConfigs;
};

AHalConfigHelper* AHalConfigHelper::mConfigHelper;
static AHalValues* (*getAHalConfigs)() = nullptr;

void AHalConfigHelper::initConfigHelper(bool isVendorEnhancedFwk)
{
    ALOGV("%s: enter", __FUNCTION__);

    void *handle = dlopen(AUDIO_CONFIGSTORE_LIB_PATH, RTLD_NOW);
    if (handle != nullptr) {
        getAHalConfigs = (AHalValues*(*)())
                     dlsym(handle, "getAudioHalExtConfigs");
        if (!getAHalConfigs) {
            ALOGE("%s: Could not find symbol: %s", __FUNCTION__, dlerror());
            dlclose(handle);
            handle = nullptr;
        }
    }

#ifdef LINUX_ENABLED
    defaultConfigs = {
        true,        /* SND_MONITOR */
        false,       /* COMPRESS_CAPTURE */
        true,        /* SOURCE_TRACK */
        true,        /* SSREC */
        true,        /* AUDIOSPHERE */
        true,        /* AFE_PROXY */
        false,       /* USE_DEEP_AS_PRIMARY_OUTPUT */
        true,        /* HDMI_EDID */
        false,       /* KEEP_ALIVE */
        false,       /* HIFI_AUDIO */
        true,        /* RECEIVER_AIDED_STEREO */
        true,        /* KPI_OPTIMIZE */
        true,        /* DISPLAY_PORT */
        true,        /* FLUENCE */
        false,       /* CUSTOM_STEREO */
        true,        /* ANC_HEADSET */
        true,        /* SPKR_PROT */
        true,        /* FM_POWER_OPT */
        false,       /* EXTERNAL_QDSP */
        false,       /* EXTERNAL_SPEAKER */
        false,       /* EXTERNAL_SPEAKER_TFA */
        false,       /* HWDEP_CAL */
        false,       /* DSM_FEEDBACK */
        true,        /* USB_OFFLOAD */
        false,       /* USB_OFFLOAD_BURST_MODE */
        false,       /* USB_OFFLOAD_SIDETONE_VOLM */
        true,        /* A2DP_OFFLOAD */
        true,        /* HFP */
        true,        /* VBAT */
        false,       /* WSA*/
        true,        /* EXT_HW_PLUGIN */
        false,       /* RECORD_PLAY_CONCURRENCY */
        true,        /* HDMI_PASSTHROUGH */
        false,       /* CONCURRENT_CAPTURE */
        false,       /* COMPRESS_IN */
        false,       /* BATTERY_LISTENER */
        false,       /* MAXX_AUDIO */
        true,        /* COMPRESS_METADATA_NEEDED */
        false,       /* INCALL_MUSIC */
        false,       /* COMPRESS_VOIP */
        true,        /* DYNAMIC_ECNS */
        false,       /* AUDIO_ZOOM */
    };
#else
    if (isVendorEnhancedFwk) {
        defaultConfigs = {
            true,        /* SND_MONITOR */
            false,       /* COMPRESS_CAPTURE */
            true,        /* SOURCE_TRACK */
            true,        /* SSREC */
            true,        /* AUDIOSPHERE */
            true,        /* AFE_PROXY */
            false,       /* USE_DEEP_AS_PRIMARY_OUTPUT */
            true,        /* HDMI_EDID */
            true,        /* KEEP_ALIVE */
            false,       /* HIFI_AUDIO */
            true,        /* RECEIVER_AIDED_STEREO */
            true,        /* KPI_OPTIMIZE */
            true,        /* DISPLAY_PORT */
            true,        /* FLUENCE */
            true,        /* CUSTOM_STEREO */
            true,        /* ANC_HEADSET */
            true,        /* SPKR_PROT */
            true,        /* FM_POWER_OPT */
            false,       /* EXTERNAL_QDSP */
            false,       /* EXTERNAL_SPEAKER */
            false,       /* EXTERNAL_SPEAKER_TFA */
            false,       /* HWDEP_CAL */
            false,       /* DSM_FEEDBACK */
            true,        /* USB_OFFLOAD */
            false,       /* USB_OFFLOAD_BURST_MODE */
            false,       /* USB_OFFLOAD_SIDETONE_VOLM */
            true,        /* A2DP_OFFLOAD */
            true,        /* HFP */
            true,        /* VBAT */
            false,       /* WSA*/
            true,        /* EXT_HW_PLUGIN */
            false,       /* RECORD_PLAY_CONCURRENCY */
            true,        /* HDMI_PASSTHROUGH */
            true,        /* CONCURRENT_CAPTURE */
            true,        /* COMPRESS_IN */
            true,        /* BATTERY_LISTENER */
            false,       /* MAXX_AUDIO */
            true,        /* COMPRESS_METADATA_NEEDED */
            true,        /* INCALL_MUSIC */
            false,       /* COMPRESS_VOIP */
            true,        /* DYNAMIC_ECNS */
            false,       /* AUDIO_ZOOM */
        };
    } else {
        defaultConfigs = {
#if SND_MONITOR_ENABLED
            true,        /* SND_MONITOR */
#else
            false,        /* SND_MONITOR */
#endif
#if COMPRESS_CAPTURE_ENABLED
            true,       /* COMPRESS_CAPTURE */
#else
            false,       /* COMPRESS_CAPTURE */
#endif
#if SOURCE_TRACKING_ENABLED
            true,       /* SOURCE_TRACK */
#else
            false,       /* SOURCE_TRACK */
#endif
#if SSREC_ENABLED
            true,       /* SSREC */
#else
            false,       /* SSREC */
#endif
#if AUDIOSPHERE_ENABLED
            true,       /* AUDIOSPHERE */
#else
            false,       /* AUDIOSPHERE */
#endif
#if AFE_PROXY_ENABLED
            true,       /* AFE_PROXY */
#else
            false,       /* AFE_PROXY */
#endif
#if USE_DEEP_AS_PRIMARY_OUTPUT_ENABLED
            true,       /* USE_DEEP_AS_PRIMARY_OUTPUT */
#else
            false,       /* USE_DEEP_AS_PRIMARY_OUTPUT */
#endif
#if HDMI_EDID_ENABLED
            true,       /* HDMI_EDID */
#else
            false,       /* HDMI_EDID */
#endif
#if KEEP_ALIVE_ENABLED
            true,       /* KEEP_ALIVE */
#else
            false,       /* KEEP_ALIVE */
#endif
#if HIFI_AUDIO_ENABLED
            true,       /* HIFI_AUDIO */
#else
            false,       /* HIFI_AUDIO */
#endif
#if RECEIVER_AIDED_STEREO_ENABLED
            true,       /* RECEIVER_AIDED_STEREO */
#else
            false,       /* RECEIVER_AIDED_STEREO */
#endif
#if KPI_OPTIMIZE_ENABLED
            true,       /* KPI_OPTIMIZE */
#else
            false,       /* KPI_OPTIMIZE */
#endif
#if DISPLAY_PORT_ENABLED
            true,       /* DISPLAY_PORT */
#else
            false,       /* DISPLAY_PORT */
#endif
#if FLUENCE_ENABLED
            true,       /* FLUENCE */
#else
            false,       /* FLUENCE */
#endif
#if CUSTOM_STEREO_ENABLED
            true,       /* CUSTOM_STEREO */
#else
            false,       /* CUSTOM_STEREO */
#endif
#if ANC_HEADSET_ENABLED
            true,       /* ANC_HEADSET */
#else
            false,       /* ANC_HEADSET */
#endif
#if DSM_FEEDBACK_ENABLED
            true,       /* DSM_FEEDBACK */
#else
            false,       /* DSM_FEEDBACK */
#endif
#if USB_OFFLOAD_ENABLED
            true,        /* USB_OFFLOAD */
#else
            false,        /* USB_OFFLOAD */
#endif
#if USB_OFFLOAD_BURST_MODE_ENABLED
            true,       /* USB_OFFLOAD_BURST_MODE */
#else
            false,       /* USB_OFFLOAD_BURST_MODE */
#endif
#if USB_OFFLOAD_SIDETONE_VOLM_ENABLED
            true,       /* USB_OFFLOAD_SIDETONE_VOLM */
#else
            false,       /* USB_OFFLOAD_SIDETONE_VOLM */
#endif
#if A2DP_OFFLOAD_ENABLED
            true,        /* A2DP_OFFLOAD */
#else
            false,        /* A2DP_OFFLOAD */
#endif
#if VBAT_MONITOR_ENABLED
            true,       /* VBAT */
#else
            false,       /* VBAT */
#endif
#if COMPRESS_METADATA_ENABLED
            true,       /* COMPRESS_METADATA_NEEDED */
#else
            false,       /* COMPRESS_METADATA_NEEDED */
#endif
#ifdef COMPRESS_VOIP_ENABLED
            true,       /* COMPRESS_VOIP */
#else
            false,       /* COMPRESS_VOIP */
#endif
#ifdef DYNAMIC_ECNS_ENABLED
            true,       /* DYNAMIC_ECNS */
#else
            false,       /* DYNAMIC_ECNS */
#endif
#ifdef SPKR_PROT_ENABLED
            true,        /* SPKR_PROT */
#else
            false,        /* SPKR_PROT */
#endif
        };
    }
#endif
}

void AHalConfigHelper::getAHalValues(AHalValues* *confValues)
{
    if (getAHalConfigs != nullptr)
        *confValues = getAHalConfigs();

    if (*confValues == nullptr) {
        ALOGI("%s: Could not retrieve flags from configstore, setting defaults",
                   __FUNCTION__);
        *confValues = &defaultConfigs;
    }
}

extern "C" {

void audio_extn_ahal_config_helper_init(bool is_vendor_enhanced_fwk)
{
    AHalConfigHelper* confInstance = AHalConfigHelper::getAHalConfInstance();
    if (confInstance != nullptr)
        confInstance->initConfigHelper(is_vendor_enhanced_fwk);
}

void audio_extn_get_feature_values(AHalValues* *confValues)
{
    AHalConfigHelper* confInstance = AHalConfigHelper::getAHalConfInstance();
    if (confInstance != nullptr)
        confInstance->getAHalValues(confValues);
}

} // extern C
