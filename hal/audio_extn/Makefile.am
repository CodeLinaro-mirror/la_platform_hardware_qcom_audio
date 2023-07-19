AM_CFLAGS = -I $(top_srcdir)/hal \
        -I $(top_srcdir)/hal/audio_extn \
        -I $(top_srcdir)/hal/voice_extn \
        -I $(PKG_CONFIG_SYSROOT_DIR)/usr/include/audio-kernel/audio \
        -I $(PKG_CONFIG_SYSROOT_DIR)/usr/include \
        -I $(top_srcdir)/hal/${TARGET_PLATFORM}

AM_CFLAGS += -DAUDIO_GKI_ENABLED
AM_CFLAGS += -DENABLE_AUDIO_LEGACY_PURE
AM_CFLAGS += -fcommon

h_sources = audio_defs.h \
            audio_extn.h \
            ../audio_hw.h \
            ../voice.h

library_include_HEADERS = $(h_sources)
library_includedir = $(includedir)

################################## SND MONITOR LIB ####################################
audio_snd_monitor_la_SOURCES = sndmonitor.c

lib_LTLIBRARIES = audio.snd.monitor.la
audio_snd_monitor_la_LIBADD = $(GLIB_LIBS) -llog -lcutils -ltinyalsa
audio_snd_monitor_la_LIBADD += -ltinycompress -laudioroute -ldl -lexpat -laudioutils
audio_snd_monitor_la_CFLAGS = $(AM_CFLAGS) $(GLIB_CFLAGS)
audio_snd_monitor_la_CFLAGS += -Dstrlcat=g_strlcat
audio_snd_monitor_la_CFLAGS += -DINT_MAX=0x7fffffff
audio_snd_monitor_la_CFLAGS += -D__unused=__attribute__\(\(__unused__\)\)
audio_snd_monitor_la_CFLAGS += -DLINUX_ENABLED $(TARGET_CFLAGS) -DAUDIO_EXTN_FORMATS_ENABLED
audio_snd_monitor_la_CFLAGS += -DNDEBUG
audio_snd_monitor_la_LDFLAGS = -module -shared -avoid-version

################################## SSR LIB ####################################
if SSR
AM_CFLAGS += -DSSR_ENABLED
AM_CFLAGS +=  -I ${WORKSPACE}/audio/mm-audio-noship/surround_sound_3mic/libsurround_3mic_proc/surround_rec_interface/inc/

audio_ssrec_la_SOURCES = ssr.c \
                         device_utils.c

lib_LTLIBRARIES += audio.ssrec.la
audio_ssrec_la_LIBADD = $(GLIB_LIBS) -llog -lcutils -ltinyalsa
audio_ssrec_la_LIBADD += -ltinycompress -laudioroute -ldl -lexpat -laudioutils
audio_ssrec_la_CFLAGS = $(AM_CFLAGS) $(GLIB_CFLAGS)
audio_ssrec_la_CFLAGS += -Dstrlcat=g_strlcat
audio_ssrec_la_CFLAGS += -DINT_MAX=0x7fffffff
audio_ssrec_la_CFLAGS += -D__unused=__attribute__\(\(__unused__\)\)
audio_ssrec_la_CFLAGS += -DLINUX_ENABLED $(TARGET_CFLAGS) -DAUDIO_EXTN_FORMATS_ENABLED
audio_ssrec_la_CFLAGS += -DNDEBUG
audio_ssrec_la_LDFLAGS = -module -shared -avoid-version
endif

################################## A2DP OFFLOAD LIB ####################################
audio_a2dp_offload_la_SOURCES = a2dp.c \
                                device_utils.c

lib_LTLIBRARIES += audio.a2dp.offload.la
audio_a2dp_offload_la_LIBADD = $(GLIB_LIBS) -llog -lcutils -ltinyalsa
audio_a2dp_offload_la_LIBADD += -ltinycompress -laudioroute -ldl -lexpat -laudioutils
audio_a2dp_offload_la_CFLAGS = $(AM_CFLAGS) $(GLIB_CFLAGS)
audio_a2dp_offload_la_CFLAGS += -Dstrlcat=g_strlcat
audio_a2dp_offload_la_CFLAGS += -DINT_MAX=0x7fffffff
audio_a2dp_offload_la_CFLAGS += -D__unused=__attribute__\(\(__unused__\)\)
audio_a2dp_offload_la_CFLAGS += -DLINUX_ENABLED $(TARGET_CFLAGS) -DAUDIO_EXTN_FORMATS_ENABLED
audio_a2dp_offload_la_CFLAGS += -DNDEBUG
audio_a2dp_offload_la_LDFLAGS = -module -shared -avoid-version

################################## HFP LIB ####################################
if HFP
AM_CFLAGS += -DHFP_ENABLED

lib_LTLIBRARIES += libhfp.la
libhfp_la_SOURCES = hfp.c \
                    device_utils.c

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

################################## SPEAKER PROTECTION LIB ####################################
if SPKR_PROT
AM_CFLAGS += -DSPKR_PROT_ENABLED

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

################################## EDID LIB ####################################
if HDMI_EDID
lib_LTLIBRARIES += audio.hdmi.edid.la
audio_hdmi_edid_la_SOURCES = edid.c
audio_hdmi_edid_la_LIBADD = $(GLIB_LIBS) -llog -lcutils -ltinyalsa
audio_hdmi_edid_la_LIBADD += -ltinycompress -laudioroute -ldl -lexpat -laudioutils
audio_hdmi_edid_la_CFLAGS = $(AM_CFLAGS) $(GLIB_CFLAGS)
audio_hdmi_edid_la_CFLAGS += -Dstrlcat=g_strlcat
audio_hdmi_edid_la_CFLAGS += -DINT_MAX=0x7fffffff
audio_hdmi_edid_la_CFLAGS += -D__unused=__attribute__\(\(__unused__\)\)
audio_hdmi_edid_la_CFLAGS += -DLINUX_ENABLED $(TARGET_CFLAGS) -DAUDIO_EXTN_FORMATS_ENABLED
audio_hdmi_edid_la_CFLAGS += -DNDEBUG
audio_hdmi_edid_la_LDFLAGS = -module -shared -avoid-version
endif

################################## COMPRESS_CAPTURE LIB ####################################
if COMPRESS_CAPTURE
AM_CFLAGS += -DCOMPRESS_CAPTURE_ENABLED
audio_compress_capture_la_SOURCES = compress_capture.c
endif
