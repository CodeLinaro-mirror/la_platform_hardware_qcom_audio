AM_CFLAGS = -I $(top_srcdir)/hal \
        -I $(top_srcdir)/hal/audio_extn \
        -I $(top_srcdir)/hal/voice_extn \
        -I $(PKG_CONFIG_SYSROOT_DIR)/usr/include/audio-kernel/audio \
        -I $(PKG_CONFIG_SYSROOT_DIR)/usr/include \
        -I $(top_srcdir)/hal/${TARGET_PLATFORM}

AM_CFLAGS += -DAUDIO_GKI_ENABLED
AM_CFLAGS += -DENABLE_AUDIO_LEGACY_PURE

h_sources = audio_defs.h \
            audio_extn.h \
            ../audio_hw.h \
            ../voice.h

################################## A2DP OFFLOAD LIB ####################################
c_sources = a2dp.c
library_include_HEADERS = $(h_sources)
library_includedir = $(includedir)

lib_LTLIBRARIES = audio.a2dp.offload.la
audio_a2dp_offload_la_SOURCES = $(c_sources)
audio_a2dp_offload_la_LIBADD = $(GLIB_LIBS) -llog -lcutils -ltinyalsa
audio_a2dp_offload_la_LIBADD += -ltinycompress -laudioroute -ldl -lexpat -laudioutils
audio_a2dp_offload_la_CFLAGS = $(AM_CFLAGS) $(GLIB_CFLAGS)
audio_a2dp_offload_la_CFLAGS += -Dstrlcat=g_strlcat
audio_a2dp_offload_la_CFLAGS += -DINT_MAX=0x7fffffff
audio_a2dp_offload_la_CFLAGS += -D__unused=__attribute__\(\(__unused__\)\)
audio_a2dp_offload_la_CFLAGS += -DLINUX_ENABLED $(TARGET_CFLAGS) -DAUDIO_EXTN_FORMATS_ENABLED
audio_a2dp_offload_la_CFLAGS += -DNDEBUG
audio_a2dp_offload_la_LDFLAGS = -module -shared -avoid-version

################################## A2DP OFFLOAD LIB ####################################

if FM_POWER_OPT
AM_CFLAGS += -DFM_POWER_OPT
c_sources += fm.c
endif

if USBAUDIO
AM_CFLAGS += -DUSB_TUNNEL_ENABLED
c_sources += usb.c
endif

if SSR
AM_CFLAGS += -DSSR_ENABLED
c_sources += ssr.c
endif

if MULTI_VOICE_SESSIONS
AM_CFLAGS += -DMULTI_VOICE_SESSION_ENABLED
c_sources += $(top_srcdir)/hal/voice_extn/voice_extn.c
endif

if COMPRESS_VOIP
AM_CFLAGS += -DCOMPRESS_VOIP_ENABLED
c_sources += $(top_srcdir)/hal/voice_extn/compress_voip.c
endif

if SPKR_PROT
AM_CFLAGS += -DSPKR_PROT_ENABLED
c_sources += spkr_protection.c
endif

if HW_VARIANTS
AM_CFLAGS += -DHW_VARIANTS_ENABLED
c_sources +=  $(top_srcdir)/hal/${TARGET_PLATFORM}/hw_info.c
endif

if COMPRESS_CAPTURE
AM_CFLAGS += -DCOMPRESS_CAPTURE_ENABLED
c_sources += compress_capture.c
endif

if DTS_EAGLE
AM_CFLAGS += -DDTS_EAGLE
c_sources += dts_eagle.c
endif

if DOLBY_DDP
AM_CFLAGS += -DDS1_DOLBY_DDP_ENABLED
c_sources += dolby.c
endif

if DS1_DOLBY_DAP
AM_CFLAGS += -DDS1_DOLBY_DAP_ENABLED
if DOLBY_DDP
c_sources += dolby.c
endif
endif

if DEV_ARBI
AM_CFLAGS += -DDEV_ARBI_ENABLED
c_sources += dev_arbi.c
endif

if SOURCE_TRACKING
AM_CFLAGS += -DSOURCE_TRACKING_ENABLED
c_sources += source_track.c
endif

if LISTEN
AM_CFLAGS += -DAUDIO_LISTEN_ENABLED
c_sources += listen.c
endif

if SOUND_TRIGGER
AM_CFLAGS += -DSOUND_TRIGGER_ENABLED
c_sources += soundtrigger.c
endif

if FLAC_SUPPORT
AM_CFLAGS += -DFLAC_OFFLOAD_ENABLED
AM_CFLAGS += -DCOMPRESS_METADATA_ENABLED
endif

if ALAC_SUPPORT
AM_CFLAGS += -DALAC_OFFLOAD_ENABLED
AM_CFLAGS += -DCOMPRESS_METADATA_ENABLED
endif

if VORBIS_SUPPORT
AM_CFLAGS += -DVORBIS_OFFLOAD_ENABLED
AM_CFLAGS += -DCOMPRESS_METADATA_ENABLED
endif

if WMA_SUPPORT
AM_CFLAGS += -DWMA_OFFLOAD_ENABLED
AM_CFLAGS += -DCOMPRESS_METADATA_ENABLED
endif

if COMPRESS_INPUT
AM_CFLAGS += -DCOMPRESS_INPUT_ENABLED
c_sources += compress_in.c
endif

if COMPRESS_PASSTHROUGH
AM_CFLAGS += -DHDMI_PASSTHROUGH_ENABLED
c_sources += passthru.c
endif

if AUDIO_PARSER
if DTSHD_PARSER
AM_CFLAGS += -DDTSHD_PARSER_ENABLED
endif
endif

if KEEP_ALIVE
AM_CFLAGS += -DKEEP_ALIVE_ENABLED
c_sources += keep_alive.c
endif

if GEF
AM_CFLAGS += -DAUDIO_GENERIC_EFFECT_FRAMEWORK_ENABLED
c_sources += gef.c
endif

if APTX_DECODER
AM_CFLAGS += -DAPTX_DECODER_ENABLED
endif

if AFE_PROXY
AM_CFLAGS += -DAFE_PROXY_ENABLED
endif

if ADSP_HDLR
AM_CFLAGS += -DAUDIO_EXTN_ADSP_HDLR_ENABLED
c_sources += adsp_hdlr.c
endif

if AUDIO_IP_HDLR
AM_CFLAGS += -DAUDIO_EXTN_IP_HDLR_ENABLED
c_sources += ip_hdlr_intf.c
endif

if QAF_SUPPORT
AM_CFLAGS += -DQAF_EXTN_ENABLED
c_sources += qaf.c
endif

if RUN_KEEP_ALIVE_IN_ARM_FFV
AM_CFLAGS += -DRUN_KEEP_ALIVE_IN_ARM_FFV
endif

if AUDIO_HW_FFV
AM_CFLAGS += -DFFV_ENABLED \
             -I $(PKG_CONFIG_SYSROOT_DIR)/usr/include/ffv/
c_sources += ffv.c
endif

if CUSTOM_STEREO
AM_CFLAGS += -DCUSTOM_STEREO_ENABLED
endif

if INSTANCE_ID
AM_CFLAGS += -DINSTANCE_ID_ENABLED
endif

if LL_AS_PRIMARY_OUTPUT
AM_CFLAGS += -DUSE_LL_AS_PRIMARY_OUTPUT
endif

if HFP
AM_CFLAGS += -DHFP_ENABLED
c_sources += hfp.c
lib_LTLIBRARIES += libhfp.la
libhfp_la_SOURCES = $(c_sources)
libhfp_la_LIBADD = $(GLIB_LIBS) -llog -lcutils -ltinyalsa
libhfp_la_LIBADD += -ltinycompress -laudioroute -ldl -lexpat -laudioutils
libhfp_la_LIBADD += -lm -lc -lresolv

if AUDIO_PARSER
libhfp_la_LIBADD += -laudioparsers
endif

libhfp_la_CFLAGS = $(AM_CFLAGS) $(GLIB_CFLAGS)
libhfp_la_CFLAGS += -Dstrlcat=g_strlcat
libhfp_la_CFLAGS += -DINT_MAX=0x7fffffff
libhfp_la_CFLAGS += -DULONG_MAX=\(__LONG_MAX__*2UL+1UL\)
libhfp_la_CFLAGS += -DPATH_MAX=4096
libhfp_la_CFLAGS += -D__unused=__attribute__\(\(__unused__\)\)
libhfp_la_CFLAGS += -DLINUX_ENABLED $(TARGET_CFLAGS) -DAUDIO_EXTN_FORMATS_ENABLED
libhfp_la_CFLAGS += -DNDEBUG
libhfp_la_CFLAGS += -D_GNU_SOURCE
if HAL_PATH_SPECIFIED
libhfp_la_CFLAGS += -DHAL_LIBRARY_PATH=\"@HAL_PATH@\"
endif
libhfp_la_LDFLAGS = -module -shared -avoid-version
endif

if SPKR_PROT
AM_CFLAGS += -DSPKR_PROT_ENABLED
library_include_HEADERS = $(h_sources)
library_includedir = $(includedir)
lib_LTLIBRARIES += audio.spkr.prot.la
audio_spkr_prot_la_SOURCES = spkr_protection.c
audio_spkr_prot_la_LIBADD = $(GLIB_LIBS) -llog -lcutils -ltinyalsa
audio_spkr_prot_la_LIBADD += -ltinycompress -laudioroute -ldl -lexpat -laudioutils
audio_spkr_prot_la_CFLAGS = $(AM_CFLAGS) $(GLIB_CFLAGS)
audio_spkr_prot_la_CFLAGS += -Dstrlcat=g_strlcat
audio_spkr_prot_la_CFLAGS += -DINT_MAX=0x7fffffff
audio_spkr_prot_la_CFLAGS += -D__unused=__attribute__\(\(__unused__\)\)
audio_spkr_prot_la_CFLAGS += -DLINUX_ENABLED $(TARGET_CFLAGS) -DAUDIO_EXTN_FORMATS_ENABLED
audio_spkr_prot_la_CFLAGS += -DNDEBUG
audio_spkr_prot_la_LDFLAGS = -module -shared -avoid-version
endif
