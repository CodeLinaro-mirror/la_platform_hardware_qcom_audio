/*
** Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
** SPDX-License-Identifier: BSD-3-Clause-Clear
**/


#include "qahw_usb_test.h"

#define SOUND_CARD_NUM 1
struct pcm *plbk_pcm_hndl = NULL;
struct pcm *rec_pcm_hndl = NULL;
char *usb_buffer = NULL;
unsigned int usb_buf_size = 0;
char *usb_buf_ptr = NULL;
pthread_mutex_t read_write_lock;
pthread_cond_t read_write_cond;


int check_param(struct pcm_params *params, unsigned int param, unsigned int value,
                 char *param_name, char *param_unit)
{
    unsigned int min;
    unsigned int max;
    int is_within_bounds = 1;

    min = pcm_params_get_min(params, param);
    if (value < min) {
        fprintf(stderr, "%s is %u%s, device only supports >= %u%s\n", param_name, value,
                param_unit, min, param_unit);
        is_within_bounds = 0;
    }
    max = pcm_params_get_max(params, param);
    if (value > max) {
        fprintf(stderr, "%s is %u%s, device only supports <= %u%s\n", param_name, value,
                param_unit, max, param_unit);
        is_within_bounds = 0;
    }

    return is_within_bounds;
}


int sample_is_playable(unsigned int card, unsigned int device, unsigned int channels,
                        unsigned int rate, unsigned int bits, unsigned int period_size,
                        unsigned int period_count)
{
    struct pcm_params *params;
    int can_play;

    params = pcm_params_get(card, device, PCM_OUT);
    if (params == NULL) {
        fprintf(stderr, "Unable to open PCM device %u.\n", device);
        return 0;
    }

    can_play = check_param(params, PCM_PARAM_RATE, rate, "Sample rate", "Hz");
    can_play &= check_param(params, PCM_PARAM_CHANNELS, channels, "Sample", " channels");
    can_play &= check_param(params, PCM_PARAM_SAMPLE_BITS, bits, "Bitrate", " bits");
    can_play &= check_param(params, PCM_PARAM_PERIOD_SIZE, period_size, "Period size", " frames");
    can_play &= check_param(params, PCM_PARAM_PERIODS, period_count, "Period count", " periods");

    pcm_params_free(params);

    return can_play;
}

int init_record(int period_size, int period_count) {
    int rc = 0;
    unsigned int card = 1;
    unsigned int device = 0;
    struct pcm_config config;

    fprintf(stderr, "%s:Enter \n", __func__);

    memset(&config, 0, sizeof(config));

    config.channels = 2;
    config.rate = 48000;
    config.period_size = period_size;
    config.period_count = period_count;
    config.format = PCM_FORMAT_S16_LE;
    config.start_threshold = 0;
    config.stop_threshold = 0;
    config.silence_threshold = 0;
    fprintf(stderr, "%s:card:%u, device:%u, rate= %u,channels =%u  \n", __func__, card, device,config.rate, config.channels);
    fprintf(stderr, "%s:period_size:%u, period_count:%u, format= %d \n", __func__, config.period_size,config.period_count,config.format);
    rec_pcm_hndl = pcm_open(card, device, PCM_IN, &config);
    if (!rec_pcm_hndl) {
        fprintf(stderr, "Unable to open REC PCM device %u (%s)\n",
                 device, pcm_get_error(rec_pcm_hndl));
        rc = -EBADFD;
        goto exit;
    }
    if (!pcm_is_ready(rec_pcm_hndl)) {
        fprintf(stderr, "rc node pcm not ready \n");
        rc = -EBADFD;
        goto exit;
    }
exit:
    return rc;
}

int init_playback(int period_size, int period_count) {
    unsigned int card = 1;
    unsigned int device = 0;
    struct pcm_config config;
    int rc = 0;

    fprintf(stderr, "period_size:%d, period_count:%d\n", period_size, period_count);

    memset(&config, 0, sizeof(config));
    config.channels = 2;
    config.rate = 48000;
    config.period_size = period_size;
    config.period_count = period_count;
    config.format = PCM_FORMAT_S16_LE;
    config.start_threshold = 0;
    config.stop_threshold = 0;
    config.silence_threshold = 0;

    if (!sample_is_playable(card, device, config.channels, config.rate, 16, config.period_size, config.period_count)) {
        fprintf(stderr, "%s:sample not playable  \n", __func__);
        rc = -EBADFD;
        goto exit;
    }

    plbk_pcm_hndl = pcm_open(card, device, PCM_OUT, &config);
    if (!plbk_pcm_hndl) {
        fprintf(stderr, "Unable to open PLBK PCM device %u (%s)\n",
                 device, pcm_get_error(plbk_pcm_hndl));
        rc = -EBADFD;
        goto exit;
    }
    if (!pcm_is_ready(plbk_pcm_hndl)) {
        fprintf(stderr, " plbk node pcm not ready");
        rc = -EBADFD;
        goto exit;
    }
exit:
    return rc;
}

int usb_init(int period_size, int period_count)
{
    int rc = 0;

    rc = init_record(period_size, period_count);
    if (rc) {
        fprintf(stderr, "%s: rec node pcm open/start failed \n");
    }
    rc = init_playback(period_size, period_count);

    if (rc) {
        fprintf(stderr, "%s: plbk node pcm open/start failed \n");
    }
    return rc;
}

struct pcm * get_plbk_pcm_hndl() {

    if (plbk_pcm_hndl != NULL) {
        fprintf(stderr, "plbk_pcm_hndl not null\n");
        return plbk_pcm_hndl;
    }
    return NULL;
}

struct pcm * get_rec_pcm_hndl() {
    if (rec_pcm_hndl != NULL) {
        fprintf(stderr, "rec_pcm_hndl not null\n");
        return rec_pcm_hndl;
    }
    return NULL;
}

int usb_read(){
    int rc = 0;
    int ret = 0;

    rc = pcm_read(rec_pcm_hndl, usb_buffer, usb_buf_size);
    return rc;
}

int usb_write(){
    int rc = 0;
    int ret = 0;

    rc = pcm_write(plbk_pcm_hndl, usb_buffer, usb_buf_size);
    usb_buf_ptr++;

    return rc;
}

