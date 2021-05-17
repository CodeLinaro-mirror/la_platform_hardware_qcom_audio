AM_CFLAGS = -Wno-narrowing \
        -I ${WORKSPACE}/system/media/audio_effects/include \
        -I $(top_srcdir)/hal-pal \
        -I $(top_srcdir)/hal-pal/audio_extn \
        -I $(top_srcdir)/hal/audio_extn \
        -I $(PKG_CONFIG_SYSROOT_DIR)/usr/include/audio-kernel \
        -I $(STAGING_INCDIR)

c_sources = AudioStream.cpp \
    AudioDevice.cpp \
    AudioVoice.cpp \
    audio_extn/AudioExtn.cpp \
    audio_extn/soundtrigger.cpp \
    audio_extn/Gef.cpp

#if LISTEN
#AM_CFLAGS += -DAUDIO_LISTEN_ENABLED
#AM_CFLAGS += -I ${WORKSPACE}/audio/mm-audio-external-noship/audio-listen
#c_sources += audio_extn/listen.c
#endif


if FLAC_SUPPORT
AM_CFLAGS += -DFLAC_OFFLOAD_ENABLED
AM_CFLAGS += -DCOMPRESS_METADATA_NEEDED
endif

if ALAC_SUPPORT
AM_CFLAGS += -DALAC_OFFLOAD_ENABLED
AM_CFLAGS += -DCOMPRESS_METADATA_NEEDED
endif

if VORBIS_SUPPORT
AM_CFLAGS += -DVORBIS_OFFLOAD_ENABLED
AM_CFLAGS += -DCOMPRESS_METADATA_NEEDED
endif

if WMA_SUPPORT
AM_CFLAGS += -DWMA_OFFLOAD_ENABLED
AM_CFLAGS += -DCOMPRESS_METADATA_NEEDED
endif

if AUDIO_PARSER
AM_CFLAGS += -I ${WORKSPACE}/audio/mm-audio/audio-parsers/inc/
if DTSHD_PARSER
AM_CFLAGS += -DDTSHD_PARSER_ENABLED
endif
endif

if APTX_DECODER
AM_CFLAGS += -DAPTX_DECODER_ENABLED
endif

if AFE_PROXY
AM_CFLAGS += -DAFE_PROXY_ENABLED
endif

if RUN_KEEP_ALIVE_IN_ARM_FFV
AM_CFLAGS += -DRUN_KEEP_ALIVE_IN_ARM_FFV
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

h_sources = audio_extn/audio_defs.h \
            audio_extn/audio_extn.h \
            audio_extn/AudioExtn.h \
            AudioDevice.h\
            AudioStream.h\
            AudioVoice.h

library_include_HEADERS = $(h_sources)
library_includedir = $(includedir)

lib_LTLIBRARIES = audio.primary.default.la
audio_primary_default_la_SOURCES = $(c_sources)
audio_primary_default_la_LIBADD = -lglib-2.0 -llog -lcutils -ltinyalsa
audio_primary_default_la_LIBADD += -laudioroute -ldl -lexpat -lpal
if AUDIO_PARSER
audio_primary_default_la_LIBADD += -laudioparsers
endif
audio_primary_default_la_CFLAGS = $(AM_CFLAGS) $(GLIB_CFLAGS)
audio_primary_default_la_CFLAGS += -Dstrlcat=g_strlcat -Dstrlcpy=g_strlcpy -include glib.h
audio_primary_default_la_CFLAGS += -DINT_MAX=0x7fffffff
audio_primary_default_la_CFLAGS += -D__unused=__attribute__\(\(__unused__\)\)
audio_primary_default_la_CFLAGS += -DLINUX_ENABLED $(TARGET_CFLAGS) -DAUDIO_EXTN_FORMATS_ENABLED
audio_primary_default_la_CFLAGS += -DNDEBUG
audio_primary_default_la_LDFLAGS = -module -shared -avoid-version
audio_primary_default_la_CPPFLAGS = $(AM_CFLAGS) $(GLIB_CFLAGS)
audio_primary_default_la_CPPFLAGS += -Dstrlcat=g_strlcat -Dstrlcpy=g_strlcpy -include glib.h
audio_primary_default_la_CPPFLAGS += -DINT_MAX=0x7fffffff
audio_primary_default_la_CPPFLAGS += -D__unused=__attribute__\(\(__unused__\)\)
audio_primary_default_la_CPPFLAGS += -DLINUX_ENABLED $(TARGET_CFLAGS) -DAUDIO_EXTN_FORMATS_ENABLED
audio_primary_default_la_CPPFLAGS += -DNDEBUG
