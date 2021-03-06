AM_CFLAGS = -I ${WORKSPACE}/external/tinyalsa/include \
        -I ${WORKSPACE}/system/media/audio_effects/include \
        -I ${WORKSPACE}/system/media/audio_utils/include \
        -I ${WORKSPACE}/frameworks/system/media/audio/include \
        -I $(top_srcdir)/hal \
        -I $(top_srcdir)/hal/audio_extn \
        -I $(top_srcdir)/hal/voice_extn \
        -I $(top_srcdir)/hal/${TARGET_PLATFORM}

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
################################## SND MONITOR LIB ####################################
if SND_MONITOR
c_sources += sndmonitor.c
library_include_HEADERS = $(h_sources)
library_includedir = $(includedir)

lib_LTLIBRARIES += audio.snd.monitor.la
audio_snd_monitor_la_SOURCES = $(c_sources)
audio_snd_monitor_la_LIBADD = $(GLIB_LIBS) -llog -lcutils -ltinyalsa
audio_snd_monitor_la_LIBADD += -ltinycompress -laudioroute -ldl -lexpat -laudioutils
audio_snd_monitor_la_CFLAGS = $(AM_CFLAGS) $(GLIB_CFLAGS)
audio_snd_monitor_la_CFLAGS += -Dstrlcat=g_strlcat
audio_snd_monitor_la_CFLAGS += -DINT_MAX=0x7fffffff
audio_snd_monitor_la_CFLAGS += -D__unused=__attribute__\(\(__unused__\)\)
audio_snd_monitor_la_CFLAGS += -DLINUX_ENABLED $(TARGET_CFLAGS) -DAUDIO_EXTN_FORMATS_ENABLED
audio_snd_monitor_la_CFLAGS += -DNDEBUG
audio_snd_monitor_la_LDFLAGS = -module -shared -avoid-version
endif

################################## COMPRESS CAPTURE LIB ####################################
if COMPRESS_CAPTURE
c_sources += compress_capture.c
library_include_HEADERS = $(h_sources)
library_includedir = $(includedir)

lib_LTLIBRARIES += audio.compress.capture.la
audio_compress_capture_la_SOURCES = $(c_sources)
audio_compress_capture_la_LIBADD = $(GLIB_LIBS) -llog -lcutils -ltinyalsa
audio_compress_capture_la_LIBADD += -ltinycompress -laudioroute -ldl -lexpat -laudioutils
audio_compress_capture_la_CFLAGS = $(AM_CFLAGS) $(GLIB_CFLAGS)
audio_compress_capture_la_CFLAGS += -Dstrlcat=g_strlcat
audio_compress_capture_la_CFLAGS += -DINT_MAX=0x7fffffff
audio_compress_capture_la_CFLAGS += -D__unused=__attribute__\(\(__unused__\)\)
audio_compress_capture_la_CFLAGS += -DLINUX_ENABLED $(TARGET_CFLAGS) -DAUDIO_EXTN_FORMATS_ENABLED
audio_compress_capture_la_CFLAGS += -DNDEBUG
audio_compress_capture_la_LDFLAGS = -module -shared -avoid-version
endif

################################## SSREC LIB ####################################
AM_CFLAGS +=  -I ${WORKSPACE}/audio/mm-audio-noship/surround_sound_3mic/libsurround_3mic_proc/surround_rec_interface/inc/

if SSR
c_sources += ssr.c
library_include_HEADERS = $(h_sources)
library_includedir = $(includedir)

lib_LTLIBRARIES += audio.ssrec.la
audio_ssrec_la_SOURCES = $(c_sources)
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

################################## HDMI EDID LIB ####################################
if HDMI_EDID
c_sources += edid.c
library_include_HEADERS = $(h_sources)
library_includedir = $(includedir)

lib_LTLIBRARIES += audio.hdmi.edid.la
audio_hdmi_edid_la_SOURCES = $(c_sources)
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

################################## SPKR-PROTECTION LIB ####################################
AM_CFLAGS += -DSPKR_PROT_ENABLED

c_sources += spkr_protection.c
library_include_HEADERS = $(h_sources)
library_includedir = $(includedir)

lib_LTLIBRARIES += audio.spkr.prot.la
audio_spkr_prot_la_SOURCES = $(c_sources)
audio_spkr_prot_la_LIBADD = $(GLIB_LIBS) -llog -lcutils -ltinyalsa
audio_spkr_prot_la_LIBADD += -ltinycompress -laudioroute -ldl -lexpat -laudioutils
audio_spkr_prot_la_CFLAGS = $(AM_CFLAGS) $(GLIB_CFLAGS)
audio_spkr_prot_la_CFLAGS += -Dstrlcat=g_strlcat
audio_spkr_prot_la_CFLAGS += -DINT_MAX=0x7fffffff
audio_spkr_prot_la_CFLAGS += -D__unused=__attribute__\(\(__unused__\)\)
audio_spkr_prot_la_CFLAGS += -DLINUX_ENABLED $(TARGET_CFLAGS) -DAUDIO_EXTN_FORMATS_ENABLED
audio_spkr_prot_la_CFLAGS += -DNDEBUG
audio_spkr_prot_la_LDFLAGS = -module -shared -avoid-version

