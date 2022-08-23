if BUILDSYSTEM_OPENWRT

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = qahw_api.pc
EXTRA_DIST = $(pkgconfig_DATA)

AM_CPPFLAGS := -I ./inc
AM_CFLAGS = -I$(STAGING_DIR)/usr/include/qahw

else

AM_CPPFLAGS := -I $(top_srcdir)/qahw_api/inc/
AM_CPPFLAGS += -I $(PKG_CONFIG_SYSROOT_DIR)/usr/include/mm-audio/audio-hal-server
AM_CPPFLAGS += -I $(PKG_CONFIG_SYSROOT_DIR)/usr/include/mm-audio/qahw/inc
AM_CPPFLAGS += -I $(top_srcdir)/qahw/inc

endif

h_sources = inc/qahw_api.h \
            inc/qahw_effect_audiosphere.h \
            inc/qahw_effect_bassboost.h \
            inc/qahw_effect_environmentalreverb.h \
            inc/qahw_effect_equalizer.h \
            inc/qahw_effect_presetreverb.h \
            inc/qahw_effect_virtualizer.h \
            inc/qahw_effect_visualizer.h \
            inc/qahw_effect_trumpet.h

AM_CPPFLAGS += -D__unused=__attribute__\(\(__unused__\)\)

library_include_HEADERS = $(h_sources)
library_includedir = $(includedir)/mm-audio/qahw_api/inc

lib_LTLIBRARIES     = libqahw.la
libqahw_la_SOURCES   = src/qahw_api.cpp
libqahw_la_LIBADD    = -lqahwwrapper
libqahw_la_CPPFLAGS := $(AM_CPPFLAGS)
libqahw_la_CPPFLAGS += -std=c++11 -DHAVE_PTHREADS -DHAVE_ANDROID_OS
libqahw_la_CPPFLAGS += -DDEBUG_REFS_CALLSTACK_ENABLED=0
libqahw_la_CPPFLAGS += -DNDEBUG
libqahw_la_CPPFLAGS += $(GLIB_CFLAGS) -include glib.h
libqahw_la_LDFLAGS   = -ltinyalsa -lhardware -lexpat -lcutils -llog -ldl -shared -avoid-version -lpthread -lutils $(GLIB_LIBS)

if QTI_AUDIO_SERVER_ENABLED
AM_CPPFLAGS += -DQTI_AUDIO_SERVER_ENABLED
libqahw_la_LDFLAGS +=  -lqtiaudioserver -lbinder
endif
if QAHW_V1
AM_CPPFLAGS += -DQAHW_V1
endif

if BUILDSYSTEM_OPENWRT
bin_PROGRAMS = hal_play_test

PLAY_CFLAGS = -Wno-sign-compare -Werror
PLAY_CFLAGS += -D__unused=__attribute__\(\(__unused__\)\)

if BUILDSYSTEM_OPENWRT
PLAY_INCLUDES = -I ./qahw_api/inc
PLAY_INCLUDES += -I ./qahw/inc \
                -I ./hal

AM_CFLAGS = -I$(STAGING_DIR)/usr/include/qahw
else
PLAY_INCLUDES = -I $(top_srcdir)/qahw_api/inc
PLAY_INCLUDES += -I $(top_srcdir)/qahw/inc \
                 -I $(top_srcdir)/hal
AM_CFLAGS = -I $(PKG_CONFIG_SYSROOT_DIR)/usr/include/mm-audio/qahw/inc
endif

hal_play_test_SOURCES = test/qahw_playback_test.c \
                        test/qahw_effect_test.c

hal_play_test_LDADD = libqahw.la
hal_play_test_LDADD += -lutils

if QAP
AM_CFLAGS += -DQAP

if BUILDSYSTEM_OPENWRT
AM_CFLAGS += -I$(TOPDIR)/src/audio/mm-audio/qap_wrapper/inc/
else
AM_CFLAGS += -I ${WORKSPACE}/audio/mm-audio/qap_wrapper/inc/
endif

hal_play_test_SOURCES += qap_wrapper_extn.c
hal_play_test_LDADD += -lqap_wrapper
endif

hal_play_test_CFLAGS = $(PLAY_CFLAGS) $(PLAY_INCLUDES)
hal_play_test_CFLAGS += $(AM_CFLAGS) -DLINUX_ENABLED

#hal_play_test_dolby_certification
bin_PROGRAMS += hal_play_test_dolby_certification

hal_play_test_dolby_certification_SOURCES = test/qahw_playback_test.c \
                                            test/qahw_effect_test.c

hal_play_test_dolby_certification_LDADD = -lutils
hal_play_test_dolby_certification_LDADD += libqahw.la

if QAP
hal_play_test_dolby_certification_SOURCES += test/qap_wrapper_extn.c
hal_play_test_dolby_certification_LDADD += -lqap_wrapper
endif

hal_play_test_dolby_certification_CFLAGS = $(PLAY_CFLAGS) $(PLAY_INCLUDES) -DTRUMPET_CERTIFICATION=1
hal_play_test_dolby_certification_CFLAGS += $(AM_CFLAGS) -DLINUX_ENABLED

#hal_play_rec
bin_PROGRAMS += hal_rec_test

REC_CFLAGS = $(PLAY_CFLAGS) -Dnullptr=NULL
REC_INCLUDES = $(PLAY_INCLUDES)

hal_rec_test_SOURCES = test/qahw_multi_record_test.c
hal_rec_test_CFLAGS = -Dstrlcat=g_strlcat $(GLIB_CFLAGS) -include glib.h
hal_rec_test_CFLAGS += $(REC_CFLAGS) $(REC_INCLUDES)
hal_rec_test_CFLAGS += $(AM_CFLAGS)
hal_rec_test_LDADD = -lutils $(GLIB_LIBS)
hal_rec_test_LDADD += libqahw.la

bin_PROGRAMS += trans_loopback_test

if BUILDSYSTEM_OPENWRT
trans_loopback_test_INCLUDES = -I ./qahw_api/inc
else
trans_loopback_test_INCLUDES = -I $(top_srcdir)/qahw_api/inc
endif

trans_loopback_test_SOURCES = test/trans_loopback_test.c

trans_loopback_test_CFLAGS  = $(AM_CFLAGS) -Wno-sign-compare -Werror
trans_loopback_test_CFLAGS  += $(trans_loopback_test_INCLUDES)
trans_loopback_test_LDADD = -llog  -lutils -lcutils -lm
trans_loopback_test_LDADD += libqahw.la

bin_PROGRAMS += hal_voice_test

VOICE_CFLAGS = -Wno-sign-compare -Werror
VOICE_CFLAGS += -D__unused=__attribute__\(\(__unused__\)\)

if BUILDSYSTEM_OPENWRT
VOICE_INCLUDES = -I ./qahw_api/inc
VOICE_INCLUDES += -I ./qahw/inc

AM_CFLAGS = -I$(STAGING_DIR)/usr/include/qahw
else
VOICE_INCLUDES = -I $(top_srcdir)/qahw_api/inc
VOICE_INCLUDES += -I $(top_srcdir)/qahw/inc

AM_CFLAGS = -I $(PKG_CONFIG_SYSROOT_DIR)/usr/include/mm-audio/qahw/inc
endif

hal_voice_test_SOURCES = test/qahw_voice_test.c

hal_voice_test_LDADD = -lutils
hal_voice_test_LDADD += libqahw.la

if QAHW_V1
if QAP
AM_CFLAGS += -DQAP
if BUILDSYSTEM_OPENWRT
AM_CFLAGS += -I$(TOPDIR)/src/audio/mm-audio/qap_wrapper/inc/
else
AM_CFLAGS += -I ${WORKSPACE}/audio/mm-audio/qap_wrapper/inc/
endif
hal_voice_test_SOURCES += test/qap_wrapper_extn.c
hal_voice_test_LDADD += -lqap_wrapper
endif

hal_voice_test_CFLAGS = $(VOICE_CFLAGS) $(VOICE_INCLUDES)
hal_voice_test_CFLAGS += $(AM_CFLAGS) -DLINUX_ENABLED

bin_PROGRAMS += afe_loopback_test

if BUILDSYSTEM_OPENWRT
afe_loopback_test_INCLUDES = -I ./qahw_api/inc
afe_loopback_test_INCLUDES += -I ./qahw/inc

AM_CFLAGS = -I$(STAGING_DIR)/usr/include/qahw
else
afe_loopback_test_INCLUDES = -I $(top_srcdir)/qahw_api/inc
afe_loopback_test_INCLUDES += -I $(top_srcdir)/qahw/inc
afe_loopback_test_CFLAGS = -I $(PKG_CONFIG_SYSROOT_DIR)/usr/include/mm-audio/qahw/inc
endif

afe_loopback_test_SOURCES = test/afe_loopback_test.c

afe_loopback_test_CFLAGS  = $(AM_CFLAGS) -Wno-sign-compare -Werror
afe_loopback_test_CFLAGS  += $(afe_loopback_test_INCLUDES)
afe_loopback_test_CFLAGS  += -D__unused=__attribute__\(\(__unused__\)\)
afe_loopback_test_CFLAGS += $(AM_CFLAGS) -DLINUX_ENABLED

afe_loopback_test_LDADD = -llog  -lutils -lcutils
afe_loopback_test_LDADD += libqahw.la
endif
endif

