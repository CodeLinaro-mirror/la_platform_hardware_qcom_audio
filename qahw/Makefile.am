if BUILDSYSTEM_OPENWRT

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = qahw.pc
EXTRA_DIST = $(pkgconfig_DATA)

AM_CFLAGS = -I ./inc \
            -I $(TOPDIR)/src/frameworks/include/media/audio_effects \
            -I $(TOPDIR)/src/frameworks/audio_utils/include \
            -I $(TOPDIR)/src/frameworks/include/media \
            -I $(TOPDIR)/src/hardware/libhardware/include \
            -I $(TOPDIR)/src/hardware/libhardware/include/hardware

h_sources = ./inc/qahw.h \
            ./inc/qahw_effect_api.h

qahw_sources = ./src/qahw.c \
               ./src/qahw_effect.c
else

AM_CFLAGS = -I $(top_srcdir)/inc \
            -I ${WORKSPACE}/system/media/audio_effects/include \
            -I ${WORKSPACE}/frameworks/audio_utils/include \
            -I ${WORKSPACE}/frameworks/include/media \
            -I ${WORKSPACE}/hardware/libhardware/include

h_sources = inc/qahw.h \
            inc/qahw_effect_api.h

qahw_sources = $(top_srcdir)/src/qahw.c \
               $(top_srcdir)/src/qahw_effect.c

endif

qahw_include_HEADERS = $(h_sources)
qahw_includedir = $(includedir)/mm-audio/qahw/inc

qahw_api_include_HEADERS = inc/qahw_defs.h
qahw_api_includedir = $(includedir)/mm-audio/qahw_api/inc

lib_LTLIBRARIES = libqahwwrapper.la
libqahwwrapper_la_SOURCES = $(qahw_sources)

if SVA_AUDIO_CONCURRENCY
AM_CFLAGS += -DSVA_AUDIO_CONC
endif

if QAHW_V1
AM_CFLAGS += -DQAHW_V1
endif

libqahwwrapper_la_CFLAGS  = $(AM_CFLAGS)
libqahwwrapper_la_CFLAGS += -include stddef.h
libqahwwrapper_la_CFLAGS += -Dstrlcpy=g_strlcpy $(GLIB_CFLAGS) -include glib.h
libqahwwrapper_la_CFLAGS += -D__unused=__attribute__\(\(__unused__\)\)
libqahwwrapper_la_CFLAGS += -Werror -Wall
libqahwwrapper_la_CFLAGS += -DNDEBUG
libqahwwrapper_la_LDFLAGS = -shared -avoid-version -llog -lcutils -lhardware $(GLIB_LIBS)
