AM_CFLAGS = -I ${WORKSPACE}/external/tinyalsa/include \
            -I $(PKG_CONFIG_SYSROOT_DIR)/usr/include/audio-kernel \
            -I ${WORKSPACE}/system/media/audio_effects/include \
            -I $(top_srcdir)/hal \
            -I $(top_srcdir)/hal/audio_extn

c_sources = bundle.c \
            equalizer.c \
            bass_boost.c \
            virtualizer.c \
            reverb.c \
            effect_api.c \
            effect_util.c

if AFE_PROXY
AM_CFLAGS += -DAFE_PROXY_ENABLED
endif

if HW_ACC_EFFECT
AM_CFLAGS += -DHW_ACCELERATED_EFFECTS
c_sources += hw_accelerator.c
endif

#if AUDIOSPHERE
#c_sources += asphere.c
#endif

library_include_HEADERS = $(h_sources)
library_includedir = $(includedir)

lib_LTLIBRARIES = libqcompostprocbundle.la
libqcompostprocbundle_la_SOURCES = $(c_sources)
libqcompostprocbundle_la_LIBADD = $(GLIB_LIBS) -llog -lcutils -ltinyalsa -ldl
libqcompostprocbundle_la_CFLAGS = $(AM_CFLAGS) $(GLIB_CFLAGS)
libqcompostprocbundle_la_CFLAGS += -D__unused=__attribute__\(\(__unused__\)\)
libqcompostprocbundle_la_LDFLAGS = -module -shared -avoid-version
libqcompostprocbundle_la_CPPFLAGS = $(AM_CFLAGS) $(GLIB_CFLAGS)

if ADSP_PP
ADSP_PP_AM_CFLAGS = -I $(PKG_CONFIG_SYSROOT_DIR)/usr/include/audio-kernel \
            -I $(top_srcdir)/hal

adsp_pp_c_sources = adsp_post_proc.c

lib_LTLIBRARIES += libadsppostproc.la
libadsppostproc_la_SOURCES = $(adsp_pp_c_sources)
libadsppostproc_la_LIBADD = $(GLIB_LIBS) -llog -lcutils -ltinyalsa -ldl
libadsppostproc_la_CFLAGS = $(ADSP_PP_AM_CFLAGS) $(GLIB_CFLAGS)
libadsppostproc_la_CFLAGS += -D__unused=__attribute__\(\(__unused__\)\)
libadsppostproc_la_LDFLAGS = -module -shared -avoid-version
libadsppostproc_la_CPPFLAGS = $(ADSP_PP_AM_CFLAGS) $(GLIB_CFLAGS)
endif
