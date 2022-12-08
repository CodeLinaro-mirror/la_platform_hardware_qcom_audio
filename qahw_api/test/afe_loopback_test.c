/*
* Copyright (c) 2017-2019, The Linux Foundation. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
*    * Redistributions of source code must retain the above copyright
*      notice, this list of conditions and the following disclaimer.
*    * Redistributions in binary form must reproduce the above
*      copyright notice, this list of conditions and the following
*      disclaimer in the documentation and/or other materials provided
*      with the distribution.
*    * Neither the name of The Linux Foundation nor the names of its
*      contributors may be used to endorse or promote products derived
*      from this software without specific prior written permission.
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

/* Test app to capture event updates from kernel */
/* #define LOG_NDEBUG 0 */
#include <fcntl.h>
#include <linux/netlink.h>
#include <getopt.h>
#include <pthread.h>
#include <poll.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <utils/Log.h>
#include <math.h>

#include <cutils/list.h>
#include "qahw_api.h"
#include "qahw_defs.h"

pthread_t data_event_th = -1;
pthread_attr_t data_event_attr;

qahw_module_handle_t *primary_hal_handle = NULL;

FILE * log_file = NULL;
volatile bool stop_loopback = false;
volatile bool exit_process_thread = false;
const char *log_filename = NULL;
uint32_t play_duration_in_seconds = 600,play_duration_elapsed_msec = 0,play_duration_in_msec = 0;
uint32_t source_device = AUDIO_DEVICE_IN_WIRED_HEADSET;
uint32_t sink_device = AUDIO_DEVICE_OUT_WIRED_HEADSET;
uint32_t dtmf_source_device = 0;
static float loopback_gain = 1.0;
qahw_stream_handle_t *stream;

#define AFE_SOURCE_PORT_ID 0
#define DTMF_SOURCE_PORT_ID 1

#define DEVICE_SOURCE 0
#define DEVICE_SINK 1

#define MAX_MODULE_NAME_LENGTH  100

/* Function declarations */
void usage();

void break_signal_handler(int signal __attribute__((unused)))
{
   stop_loopback = true;
   fprintf(log_file,"\nEvent thread loop exiting prematurely");
}

void close_afe_loopback()
{
        fprintf(log_file,"\nStopping current loopback session");
    qahw_stream_close(stream);
}

int open_afe_loopback() {
    int rc=0;
    struct qahw_stream_attributes attr;
    qahw_device_t devices[2];
    int num_devices = 0;

    devices[0] = sink_device;

    fprintf(log_file,"\nCreating audio patch");
    if (dtmf_source_device) {
        attr.type = QAHW_AUDIO_TONE_RX;
        num_devices = 1;
    } else {
        attr.type = QAHW_AUDIO_AFE_LOOPBACK; 
        num_devices = 2;
        devices[1] = source_device;
    }

    attr.direction = QAHW_STREAM_NONE;
    attr.attr.audio.config.format = AUDIO_FORMAT_PCM_16_BIT;
    rc = qahw_stream_open(primary_hal_handle,
                     attr,
                     num_devices,
                     devices,
                     0,
                     NULL,
                     NULL,
                     NULL,
                     &stream);
    return rc;
}

qahw_module_handle_t *load_hal(void)
{
    if (primary_hal_handle == NULL) {
        primary_hal_handle = qahw_load_module(QAHW_MODULE_ID_PRIMARY);
        if (primary_hal_handle == NULL) {
            fprintf(stderr,"failure in Loading primary HAL");
            goto exit;
        }
    }
exit:
    return primary_hal_handle;
}

/*
* this function unloads all the loaded hal modules so this should be called
* after all the stream playback are concluded.
*/
int unload_hals(void) {
    if (primary_hal_handle) {
        qahw_unload_module(primary_hal_handle);
        primary_hal_handle = NULL;
    }
    return 1;
}

int source_data_event_handler(bool enable)
{
    int status =0;
    if(enable) {
        fprintf(log_file,"\nCreate and start afe loopback session begin");
        status = open_afe_loopback();
        if(status)
        {
            fprintf(log_file,"\nCreate audio patch failed with status %d",status);
            close_afe_loopback();
        }
    } else {
        close_afe_loopback();
    }
    fprintf(log_file,"\nCreate and start afe loopback session end");
    return status;
}

void process_loopback_session(void *ptr)
{
    int rc = 0;
    qahw_param_payload param_payload;

    fprintf(log_file,"\nEvent thread loop");
    if (source_data_event_handler(1))
        return;
    if (dtmf_source_device) {
             param_payload.tone_gen_params.enable = 1;
             param_payload.tone_gen_params.nr_tone_freq = 2;
             param_payload.tone_gen_params.freq[0] = 697;
             param_payload.tone_gen_params.freq[1] = 1209;
             param_payload.tone_gen_params.gain = loopback_gain;
             param_payload.tone_gen_params.duration_ms = play_duration_in_msec;
             qahw_stream_set_parameters(stream, QAHW_PARAM_TONE_GEN, &param_payload);
    } else {
        rc = qahw_stream_start(stream);

        if (rc) {
            fprintf(stderr, "Could not start stream.\n");
            goto loop_exit;
        }
    }

    while(!stop_loopback) {
         usleep(100*1000);
         play_duration_elapsed_msec += 100;
         if(play_duration_in_msec <= play_duration_elapsed_msec)
         {
             stop_loopback = true;
             fprintf(log_file,"\nElapsed set playback duration %d seconds, exiting test",play_duration_in_msec/1000);
         }
    }
    if (dtmf_source_device) {
             param_payload.tone_gen_params.enable = 0;
             qahw_stream_set_parameters(stream, QAHW_PARAM_TONE_GEN, &param_payload);
    } else {
        rc = qahw_stream_stop(stream);

        if (rc)
            fprintf(stderr, "Could not stop stream.\n");
    }
loop_exit:
    source_data_event_handler(0);
    fprintf(log_file,"\nStop afe loopback done");
    exit_process_thread = true;
}

int main(int argc, char *argv[]) {

    int status = 0;
    log_file = stdout;

    struct option long_options[] = {
        /* These options set a flag. */
        {"sink-device", required_argument,    0, 's'},
        {"dtmf-source-device", required_argument,    0, 'd'},
        {"play-duration",  required_argument,    0, 'p'},
        {"volume",        required_argument,     0, 'v'},
        {"help",          no_argument,          0, 'h'},
        {0, 0, 0, 0}
    };

    int opt = 0;
    int option_index = 0;

    while ((opt = getopt_long(argc,
                              argv,
                              "-s:d:p:v:h",
                              long_options,
                              &option_index)) != -1) {

        fprintf(log_file, "for argument %c, value is %s", opt, optarg);

        switch (opt) {
        case 's':
            sink_device = atoi(optarg);
            if (sink_device == AUDIO_DEVICE_OUT_SPEAKER)
                source_device = AUDIO_DEVICE_IN_BACK_MIC;
            else if (sink_device == AUDIO_DEVICE_OUT_WIRED_HEADSET)
                source_device = AUDIO_DEVICE_IN_WIRED_HEADSET;
            else if (sink_device == AUDIO_DEVICE_OUT_EARPIECE)
                source_device = AUDIO_DEVICE_IN_BUILTIN_MIC;
            else return -EINVAL;
            break;
        case 'd':
            dtmf_source_device = atoi(optarg);
            source_device = 0;
            break;
        case 'p':
            play_duration_in_seconds = atoi(optarg);
            break;
        case 'v':
            loopback_gain = atoi(optarg);
            break;
        case 'h':
        default :
            usage();
            return 0;
        }
    }

    fprintf(log_file,"\nTranscode loopback test begin");
    if (play_duration_in_seconds < 0 | play_duration_in_seconds > 360000) {
            fprintf(log_file,
                    "\nPlayback duration %d invalid or unsupported(range : 1 to 360000, defaulting to 600 seconds )",
                    play_duration_in_seconds);
            play_duration_in_seconds = 600;
    }
    play_duration_in_msec = play_duration_in_seconds * 1000;

    /* Register the SIGINT to close the App properly */
    if (signal(SIGINT, break_signal_handler) == SIG_ERR) {
        fprintf(log_file, "Failed to register SIGINT:%d",errno);
        fprintf(stderr, "Failed to register SIGINT:%d",errno);
    }


    /* Load HAL */
    fprintf(log_file,"\nLoading HAL for loopback usecase begin");
    primary_hal_handle = load_hal();
    if (primary_hal_handle == NULL) {
        fprintf(log_file,"\n Failure in Loading HAL, exiting");
        /* Set the exit_process_thread flag for exiting test */
        exit_process_thread = true;
        goto exit_afe_loopback_test;
    }
    fprintf(log_file,"\nLoading HAL for loopback usecase done");

    pthread_attr_init(&data_event_attr);
    fprintf(log_file,"\nData thread init done");
    pthread_attr_setdetachstate(&data_event_attr, PTHREAD_CREATE_JOINABLE);
    fprintf(log_file,"\nData thread setdetachstate done");

    fprintf(log_file,"\nData thread create");
    pthread_create(&data_event_th, &data_event_attr,
                       (void *) process_loopback_session, NULL);
    fprintf(log_file,"\nMain thread loop exit");

exit_afe_loopback_test:
    /* Wait for process thread to exit */
    while (!exit_process_thread) {
        usleep(10*1000);
    }
    fprintf(log_file,"\nJoining loopback thread");
    status = pthread_join(data_event_th, NULL);
    fprintf(log_file, "\n thread join completed, status:%d ", status);
    exit_process_thread = false;

    fprintf(log_file,"\nUnLoading HAL for loopback usecase begin");
    unload_hals();
    fprintf(log_file,"\nUnLoading HAL for loopback usecase end");

    fprintf(log_file,"\nTranscode loopback test end");
    return 0;
}

void usage() {
    fprintf(log_file,"\nUsage : afe_loopback_test\n"
                           " -s <sink_device_id>\n"
                           " -d <dtmf as source(range 0 and 1)>\n"
                           " -p <duration_in_seconds>\n"
                           " -v <volume>\n"
                           " -h <help>\n");
    fprintf(log_file,"\nExample to play for 1 minute on speaker device with volume unity: afe_loopback_test -p 60 -s 2");
    fprintf(log_file,"\nExample to play for 5 minutes on headphone device: afe_loopback_test -p 300 -s 4 ");
    fprintf(log_file,"\nExample to play dtmf tone for 5 minutes on headphone device: afe_loopback_test -p 300 -s 4 -d 1 -v 100");
    fprintf(log_file,"\nHelp : afe_loopback_test -h");
}
