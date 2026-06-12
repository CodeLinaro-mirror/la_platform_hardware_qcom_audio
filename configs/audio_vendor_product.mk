#Audio product definitions 
PRODUCT_PACKAGES += $(AUDIO_GENERIC_MODULES)

PRODUCT_PACKAGES_DEBUG += $(MM_AUDIO_DBG)

#MM_AUDIO product packages
MM_AUDIO += audiod
MM_AUDIO += libacdbloader
MM_AUDIO += libalsautils
MM_AUDIO += libaudcal
MM_AUDIO += libaudioalsa
MM_AUDIO += libaudioparsers
MM_AUDIO += libaudioconfigstore
MM_AUDIO += libcsd-client
MM_AUDIO += lib_iec_60958_61937
MM_AUDIO += libmm-audio-resampler
MM_AUDIO += libstagefright_soft_qtiflacdec
MM_AUDIO += QCAudioManager
MM_AUDIO += liblistensoundmodel
MM_AUDIO += liblistensoundmodel2
MM_AUDIO += liblsmclient
MM_AUDIO += libcapiv2svacnn
MM_AUDIO += libcapiv2vop
MM_AUDIO += libcapiv2svarnn
MM_AUDIO += liblistensoundmodel2vendor
MM_AUDIO += libsvacnnvendor
MM_AUDIO += libsvarnncnnvendor
MM_AUDIO += libcapiv2svacnnvendor
MM_AUDIO += libcapiv2vopvendor
MM_AUDIO += libcapiv2svarnnvendor
MM_AUDIO += libpdksvavendor
MM_AUDIO += liblisten
MM_AUDIO += liblistenhardware
MM_AUDIO += STApp
MM_AUDIO += libqtigef
MM_AUDIO += libqcbassboost
MM_AUDIO += libqcvirt
MM_AUDIO += libqcreverb
MM_AUDIO += libasphere
MM_AUDIO += audio_effects.conf
MM_AUDIO += ftm_test_config
MM_AUDIO += libFlacSwDec
MM_AUDIO += libAlacSwDec
MM_AUDIO += libApeSwDec
MM_AUDIO += libMpeghSwEnc
MM_AUDIO += libdsd2pcm
MM_AUDIO += audioflacapp
MM_AUDIO += libqct_resampler
MM_AUDIO += libaudiodevarb
MM_AUDIO += audiod
MM_AUDIO += libsmwrapper
MM_AUDIO += libadpcmdec
MM_AUDIO += libmulawdec
MM_AUDIO += sound_trigger.primary.$(TARGET_BOARD_PLATFORM)
MM_AUDIO += sound_trigger_test
MM_AUDIO += libhwdaphal
MM_AUDIO += libqcomvisualizer
MM_AUDIO += libqcomvoiceprocessing
MM_AUDIO += libqcompostprocbundle
MM_AUDIO += libqvop-service
MM_AUDIO += libqvop-algo-jni.qti
MM_AUDIO += qvop-daemon
MM_AUDIO += VoicePrintSDK
MM_AUDIO += libadm
MM_AUDIO += libsurround_3mic_proc
MM_AUDIO += surround_sound_rec_AZ.cfg
MM_AUDIO += surround_sound_rec_5.1.cfg
MM_AUDIO += libdrc
MM_AUDIO += drc_cfg_AZ.txt
MM_AUDIO += drc_cfg_5.1.txt
MM_AUDIO += libgcs-osal
MM_AUDIO += libgcs-calwrapper
MM_AUDIO += libgcs-ipc
MM_AUDIO += libgcs
MM_AUDIO += noisesample.bin
MM_AUDIO += antispoofing.bin
MM_AUDIO += libshoebox
MM_AUDIO += libdolby_ms12_wrapper
MM_AUDIO += silence.ac3
MM_AUDIO += libaudio_ip_handler
MM_AUDIO += libsndmonitor
MM_AUDIO += libcomprcapture
MM_AUDIO += libssrec
MM_AUDIO += libhdmiedid
MM_AUDIO += libspkrprot
MM_AUDIO += libcirrusspkrprot
MM_AUDIO += liba2dpoffload
MM_AUDIO += libexthwplugin
MM_AUDIO += libhfp
MM_AUDIO += libhdmipassthru
MM_AUDIO += libbatterylistener
MM_AUDIO += libhwdepcal
MM_AUDIO += libmediaplayerservice
MM_AUDIO += libaudiohal_deathhandler
MM_AUDIO += libstagefright_httplive
MM_AUDIO += libautohal
MM_AUDIO += MTP_Bluetooth_cal.acdb
MM_AUDIO += MTP_Codec_cal.acdb
MM_AUDIO += MTP_General_cal.acdb
MM_AUDIO += MTP_Global_cal.acdb
MM_AUDIO += MTP_Handset_cal.acdb
MM_AUDIO += MTP_Hdmi_cal.acdb
MM_AUDIO += MTP_Headset_cal.acdb
MM_AUDIO += MTP_Speaker_cal.acdb
MM_AUDIO += MTP_workspaceFile.qwsp
MM_AUDIO += QRD_Bluetooth_cal.acdb
MM_AUDIO += QRD_Codec_cal.acdb
MM_AUDIO += QRD_General_cal.acdb
MM_AUDIO += QRD_Global_cal.acdb
MM_AUDIO += QRD_Handset_cal.acdb
MM_AUDIO += QRD_Hdmi_cal.acdb
MM_AUDIO += QRD_Headset_cal.acdb
MM_AUDIO += QRD_Speaker_cal.acdb
MM_AUDIO += QRD_workspaceFile.qwsp

MM_AUDIO += CDP_Bluetooth_cal.acdb
MM_AUDIO += CDP_Codec_cal.acdb
MM_AUDIO += CDP_General_cal.acdb
MM_AUDIO += CDP_Global_cal.acdb
MM_AUDIO += CDP_Handset_cal.acdb
MM_AUDIO += CDP_Hdmi_cal.acdb
MM_AUDIO += CDP_Headset_cal.acdb
MM_AUDIO += CDP_Speaker_cal.acdb
MM_AUDIO += CDP_workspaceFile.qwsp

MM_AUDIO += IDP_Yupik_Bluetooth_cal.acdb
MM_AUDIO += IDP_Yupik_Codec_cal.acdb
MM_AUDIO += IDP_Yupik_General_cal.acdb
MM_AUDIO += IDP_Yupik_Global_cal.acdb
MM_AUDIO += IDP_Yupik_Handset_cal.acdb
MM_AUDIO += IDP_Yupik_Hdmi_cal.acdb
MM_AUDIO += IDP_Yupik_Headset_cal.acdb
MM_AUDIO += IDP_Yupik_Speaker_cal.acdb
MM_AUDIO += IDP_Yupik_workspaceFile.qwsp

MM_AUDIO += QRD_Yupik_Bluetooth_cal.acdb
MM_AUDIO += QRD_Yupik_Codec_cal.acdb
MM_AUDIO += QRD_Yupik_General_cal.acdb
MM_AUDIO += QRD_Yupik_Global_cal.acdb
MM_AUDIO += QRD_Yupik_Handset_cal.acdb
MM_AUDIO += QRD_Yupik_Hdmi_cal.acdb
MM_AUDIO += QRD_Yupik_Headset_cal.acdb
MM_AUDIO += QRD_Yupik_Speaker_cal.acdb
MM_AUDIO += QRD_Yupik_workspaceFile.qwsp

ifneq (,$(call is-board-platform-in-list2,lahaina))
MM_AUDIO += fai__4.8.8_0.0__3.0.0_0.0__3.1.2_0.0__3.2.0_0.1__eai_1.10.pmd
endif

ifeq ($(ENABLE_HYP), true)
MM_AUDIO += amfsservice
endif

#MM_AUDIO_DBG
MM_AUDIO_DBG += libstagefright_soft_ddpdec
MM_AUDIO_DBG += libsurround_proc
MM_AUDIO_DBG += surround_sound_headers
MM_AUDIO_DBG += filter1i.pcm
MM_AUDIO_DBG += filter1r.pcm
MM_AUDIO_DBG += filter2i.pcm
MM_AUDIO_DBG += filter2r.pcm
MM_AUDIO_DBG += filter3i.pcm
MM_AUDIO_DBG += filter3r.pcm
MM_AUDIO_DBG += filter4i.pcm
MM_AUDIO_DBG += filter4r.pcm
MM_AUDIO_DBG += mm-audio-ftm
MM_AUDIO_DBG += mm-audio-alsa-test
MM_AUDIO_DBG += avs_test_ker.ko
MM_AUDIO_DBG += libsrsprocessing_libs
MM_AUDIO_DBG += libsrsprocessing
MM_AUDIO_DBG += libacdbrtac
MM_AUDIO_DBG += libadiertac

PRODUCT_PACKAGES += $(MM_AUDIO)

PRODUCT_PACKAGES_DEBUG += $(MM_AUDIO_DBG)


#----------------------------------------------------------------------
# audio specific
#----------------------------------------------------------------------
TARGET_USES_AOSP := false
TARGET_USES_AOSP_FOR_AUDIO := false

ifeq ($(TARGET_USES_QMAA_OVERRIDE_AUDIO), false)
ifeq ($(TARGET_USES_QMAA),true)
AUDIO_USE_STUB_HAL := true
TARGET_USES_AOSP_FOR_AUDIO := true
endif
endif
ifeq ($(AUDIO_USE_STUB_HAL), true)
-include $(TOPDIR)vendor/qcom/opensource/audio-hal/primary-hal/configs/common/default.mk
else
# Audio hal configuration file
-include $(TOPDIR)vendor/qcom/opensource/audio-hal/primary-hal/configs/$(TARGET_BOARD_PLATFORM)/$(TARGET_BOARD_PLATFORM).mk
endif

$(warning audio check QC_HWASAN: $(QC_HWASAN) sanitize_target $(SANITIZE_TARGET))
$(call add_soong_config_namespace,vendor_audio_hwasan_config)
ifneq ($(filter audio, $(QC_HWASAN)),)
$(warning audio hwasan enabled at module level)
AUDIO_FEATURE_USE_HWASAN_ARTIFACTS := true
PRODUCT_HWASAN_INCLUDE_PATHS += \
    vendor/qcom/opensource/audio-hal
endif

# Pro Audio feature
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.audio.pro.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.audio.pro.xml

SOONG_CONFIG_qtiaudio_var00 := false
SOONG_CONFIG_qtiaudio_var11 := false
SOONG_CONFIG_qtiaudio_var22 := false
SOONG_CONFIG_qtiaudio_hwasan := false

ifneq ($(BUILD_AUDIO_TECHPACK_SOURCE), true)
    SOONG_CONFIG_qtiaudio_var00 := true
    SOONG_CONFIG_qtiaudio_var11 := true
    SOONG_CONFIG_qtiaudio_var22 := true
endif
ifeq (,$(wildcard $(QCPATH)/mm-audio-noship))
    SOONG_CONFIG_qtiaudio_var11 := true
endif
ifeq (,$(wildcard $(QCPATH)/mm-audio))
    SOONG_CONFIG_qtiaudio_var22 := true
endif

ifneq ($(filter hwaddress,$(SANITIZE_TARGET)),)
$(warning audio hwasan enabled at target level)
AUDIO_FEATURE_USE_HWASAN_ARTIFACTS := true
SOONG_CONFIG_qtiaudio_hwasan := true
endif

# this feature flag is only set when hwasan is enabled (local or global)
ifeq ($(AUDIO_FEATURE_USE_HWASAN_ARTIFACTS), true)
$(warning audio use hwasan artifacts)
$(call add_soong_config_var_value,vendor_audio_hwasan_config,use_hwasan,true)
else
$(call add_soong_config_var_value,vendor_audio_hwasan_config,use_hwasan,false)
endif
