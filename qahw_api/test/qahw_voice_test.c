/*
* Copyright (c) 2019-2020, The Linux Foundation. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above
*       copyright notice, this list of conditions and the following
*       disclaimer in the documentation and/or other materials provided
*       with the distribution.
*     * Neither the name of The Linux Foundation nor the names of its
*       contributors may be used to endorse or promote products derived
*       from this software without specific prior written permission.
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

/*
** Changes from Qualcomm Innovation Center are provided under the following license:
** Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted (subject to the limitations in the
** disclaimer below) provided that the following conditions are met:
**
**    * Redistributions of source code must retain the above copyright
**      notice, this list of conditions and the following disclaimer.
**
**    * Redistributions in binary form must reproduce the above
**      copyright notice, this list of conditions and the following
**      disclaimer in the documentation and/or other materials provided
**      with the distribution.
**
**    * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
**      contributors may be used to endorse or promote products derived
**      from this software without specific prior written permission.
**
** NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
** GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
** HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
** WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
** MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
** ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
** GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
** INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
** IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
** OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
** IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**/

/* Test app for voice call */

#include "qahw_voice_test.h"


#define ID_RIFF 0x46464952
#define ID_WAVE 0x45564157
#define ID_FMT  0x20746d66
#define ID_DATA 0x61746164
#define QAHW_KV_PAIR_LENGTH 255

#define FORMAT_PCM 1
#define WAV_HEADER_LENGTH_MAX 128
#define FORMAT_DESCRIPTOR_SIZE 12
#define SUBCHUNK1_SIZE(x) ((8) + (x))
#define SUBCHUNK2_SIZE 8
#define MAX_BUFFER_SIZE 7680

int first_usb_read_done = 0;
int first_usb_write_done = 0;
int thread_state;
int tflag = 0;
int read_started = 0;
qahw_stream_handle_t* hal_out_handle = NULL;
pthread_mutex_t usb_lock;
pthread_cond_t usb_read_cond;
pthread_cond_t usb_write_cond;
pthread_t usb_tid;
char *buffer_pointer =NULL;
char *usb_data_ptr = NULL;

voice_stream_config stream_params;
volatile bool stop = false;
volatile bool stop_dl = false;
void *context = NULL;
int capturing = 1;
unsigned int usb_rec_buf_size = 400;
char *usb_rec_buffer = NULL;
int threadsWaiting = 0;

typedef struct {
    char eventType[50];
} thread_event_type;
typedef struct Node {
    void* data;
    int buffer_size;
    struct Node* next;
} Node;

typedef struct {
    Node* head;
    Node* tail;
    pthread_mutex_t mutex;
    pthread_cond_t condNotEmpty;
} LinkedList;

LinkedList recLinkedList;
LinkedList playLinkedlist;

void initLinkedList(LinkedList* list) {
    list->head = NULL;
    list->tail = NULL;
    pthread_mutex_init(&list->mutex, NULL);
    pthread_cond_init(&list->condNotEmpty, NULL);
    return;
}

static void* manage_thread_event(LinkedList* list, void *event ) {
    thread_event_type *thread_event = (thread_event_type *)event;
    int ret = 0;

    pthread_mutex_lock(&list->mutex);

    if (strcmp(thread_event->eventType, "WriteWait") == 0) {
        threadsWaiting++;
        ret = pthread_cond_wait(&list->condNotEmpty, &list->mutex);
        threadsWaiting--;
    }

    if (strcmp(thread_event->eventType, "WriteSignal") == 0) {
        if (threadsWaiting > 0) {
            pthread_cond_signal(&list->condNotEmpty);
        }
    }
    pthread_mutex_unlock(&list->mutex);
    return NULL;
}

Node* removeFromHead(LinkedList* list){
    void* return_data = NULL;
    Node* nodeToRemove = NULL;
    thread_event_type *t_type = NULL;

    pthread_mutex_lock(&list->mutex);
    t_type = (thread_event_type*) (malloc(sizeof( thread_event_type)));
    if (!t_type){
        fprintf(stderr, "%s:thread event malloc failed %s \n", __func__, strerror(errno));
        return NULL;
    }

    if (list->head == NULL) {
        pthread_mutex_unlock(&list->mutex);
        fprintf(stderr, "%s:waiting for data \n", __func__);
        snprintf(t_type->eventType, sizeof("WriteWait"), "%s", "WriteWait");
        manage_thread_event(list, t_type);
        pthread_mutex_lock(&list->mutex);
    }
    nodeToRemove = list->head;
    if (nodeToRemove != NULL) {
        list->head = nodeToRemove->next;
        if (list->head == NULL) {
            list->tail = NULL;
        }
    } else {
        fprintf(stderr, "%s:linked list empty \n", __func__);
    }
    pthread_mutex_unlock(&list->mutex);
    free(t_type);
    t_type = NULL;
    return nodeToRemove;
}

void addToTail(LinkedList* list, void* data, int dataLength){
    void* inputData = NULL;
    Node* newNode = NULL;
    thread_event_type *t_type = NULL;

    if (dataLength ==0 ) {
        fprintf(stderr, "%s: Empty data \n", __func__);
        return;
    }
    pthread_mutex_lock(&list->mutex);
    t_type = (thread_event_type*) (malloc(sizeof( thread_event_type)));
    if (!t_type){
        fprintf(stderr, "%s:thread event malloc failed %s \n", __func__, strerror(errno));
        return;
    }

    inputData = calloc(1, dataLength);
    if (inputData == NULL){
        fprintf(stderr, "%s:inputdata calloc failed\n",__func__);
        free(t_type);
        t_type = NULL;
        return;
    }

    memcpy(inputData, data, dataLength);
    fprintf(stderr, "%s: inputData: %s \n", __func__, inputData);
    newNode= (Node*)calloc(1, sizeof(Node));
    newNode->data = inputData;
    newNode->buffer_size = dataLength;
    newNode->next = NULL;

    if(list->tail == NULL) {
        fprintf(stderr, "%s: %d\n", __func__, __LINE__);
        list->head = newNode;
        list->tail = newNode;
    } else {
        list->tail->next = newNode;
        list->tail = newNode;
    }
    pthread_mutex_unlock(&list->mutex);
    snprintf(t_type->eventType, sizeof("WriteSignal"), "%s", "WriteSignal");
    manage_thread_event(list, t_type);
    free(t_type);
    t_type = NULL;
    return;
}

struct wav_header {
    uint32_t riff_id;
    uint32_t riff_sz;
    uint32_t riff_fmt;
    uint32_t fmt_id;
    uint32_t fmt_sz;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;       /* sample_rate * num_channels * bps / 8 */
    uint16_t block_align;     /* num_channels * bps / 8 */
    uint16_t bits_per_sample;
    uint32_t data_id;
    uint32_t data_sz;
};

static void init_stream(void) {
    stream_params.vsid = "11C05000";
    stream_params.qahw_mod_handle = NULL;
    stream_params.call_length = -1; /*infinite*/
    stream_params.multi_call = 1;
    stream_params.output_device[0] = AUDIO_DEVICE_OUT_WIRED_HEADSET;
    stream_params.output_device[1] = AUDIO_DEVICE_IN_BUILTIN_MIC;
    stream_params.in_call_rec = false;
    stream_params.in_call_playback = false;
    stream_params.in_dl_call_playback = false;
    stream_params.hpcm = false;
    stream_params.hpcm_tp = 2;
    stream_params.hpcm_sr = 8000;
    stream_params.tp_dir = 0;
    stream_params.rec_file = "/data/default_rec.wav";
    stream_params.playback_file = NULL;
    stream_params.vol = .75;
    stream_params.mute = false;
    stream_params.mute_dir = 0;
    stream_params.tty_mode = 0;
    stream_params.dtmf_gen_enable = 0;
    stream_params.dtmf_gain = 100;
    stream_params.dtmf_detect_enable = 0;
    stream_params.file_type = FILE_WAV;
    stream_params.stream_type = 1;
    pthread_mutex_init(&stream_params.write_lock, (const pthread_mutexattr_t *)NULL);
    pthread_cond_init(&stream_params.write_cond, (const pthread_condattr_t *) NULL);
    pthread_mutex_init(&stream_params.drain_lock, (const pthread_mutexattr_t *)NULL);
    pthread_cond_init(&stream_params.drain_cond, (const pthread_condattr_t *) NULL);
    pthread_mutex_init(&stream_params.write_lock_dl, (const pthread_mutexattr_t *)NULL);
    pthread_cond_init(&stream_params.write_cond_dl, (const pthread_condattr_t *) NULL);
    pthread_mutex_init(&stream_params.drain_lock_dl, (const pthread_mutexattr_t *)NULL);
    pthread_cond_init(&stream_params.drain_cond_dl, (const pthread_condattr_t *) NULL);
    pthread_mutex_init(&usb_lock, (const pthread_mutexattr_t *)NULL);
    pthread_cond_init(&usb_read_cond, (const pthread_condattr_t *) NULL);
    pthread_cond_init(&usb_write_cond, (const pthread_condattr_t *) NULL);
}

static void deinit_streams(void)
{
    pthread_cond_destroy(&stream_params.write_cond);
    pthread_mutex_destroy(&stream_params.write_lock);
    pthread_cond_destroy(&stream_params.drain_cond);
    pthread_mutex_destroy(&stream_params.drain_lock);
    pthread_cond_destroy(&stream_params.write_cond_dl);
    pthread_mutex_destroy(&stream_params.write_lock_dl);
    pthread_cond_destroy(&stream_params.drain_cond_dl);
    pthread_mutex_destroy(&stream_params.drain_lock_dl);
    pthread_mutex_destroy(&recLinkedList.mutex);
    pthread_mutex_destroy(&playLinkedlist.mutex);
}


static void sigint_handler(int sig)
{
   capturing = 0;
}
static int async_callback(qahw_stream_callback_event_t event, void *param,
                  void *cookie)
{
    uint32_t *payload = param;
    int i;

    if(cookie == NULL) {
        fprintf(stderr, "Invalid callback handle\n");
        return 0;
    }

    voice_stream_config *params = (voice_stream_config*) cookie;

    switch (event) {
    case QAHW_STREAM_CBK_EVENT_WRITE_READY:
        fprintf(stderr, "received event - QAHW_STREAM_CBK_EVENT_WRITE_READY\n");
        pthread_mutex_lock(&params->write_lock);
        pthread_cond_signal(&params->write_cond);
        pthread_mutex_unlock(&params->write_lock);
        break;
    case QAHW_STREAM_CBK_EVENT_DRAIN_READY:
        fprintf(stderr, "received event - QAHW_STREAM_CBK_EVENT_DRAIN_READY\n");
        pthread_mutex_lock(&params->drain_lock);
        params->drain_received = true;
        pthread_cond_signal(&params->drain_cond);
        pthread_mutex_unlock(&params->drain_lock);
        break;
    case QAHW_STREAM_CBK_EVENT_ADSP:
        fprintf(stderr, "received event - QAHW_STREAM_CBK_EVENT_ADSP\n");
        if (payload != NULL) {
            fprintf(stderr, "event_type %d\n", payload[0]);
            fprintf(stderr, "param_length %d\n", payload[1]);
            for (i=2; i* sizeof(uint32_t) <= payload[1]; i++)
                fprintf(stderr, "param[%d] = 0x%x\n", i, payload[i]);
        }
        break;
    case QAHW_STREAM_CBK_EVENT_ERROR:
        fprintf(stderr, "received event - QAHW_STREAM_CBK_EVENT_ERROR\n");
        stop = true;
        break;
    default:
        break;
    }
    return 0;
}

static int async_callback_dl(qahw_stream_callback_event_t event, void *param,
                  void *cookie)
{
    uint32_t *payload = param;
    int i;

    if(cookie == NULL) {
        fprintf(stderr, "Invalid callback handle\n");
        fprintf(stderr, "Invalid callback handle\n");
        return 0;
    }

    voice_stream_config *params = (voice_stream_config*) cookie;

    switch (event) {
    case QAHW_STREAM_CBK_EVENT_WRITE_READY:
        fprintf(stderr, "received event - QAHW_STREAM_CBK_EVENT_WRITE_READY\n");
        pthread_mutex_lock(&params->write_lock_dl);
        pthread_cond_signal(&params->write_cond_dl);
        pthread_mutex_unlock(&params->write_lock_dl);
        break;
    case QAHW_STREAM_CBK_EVENT_DRAIN_READY:
        fprintf(stderr, "received event - QAHW_STREAM_CBK_EVENT_DRAIN_READY\n");
        pthread_mutex_lock(&params->drain_lock_dl);
        params->drain_received_dl = true;
        pthread_cond_signal(&params->drain_cond_dl);
        pthread_mutex_unlock(&params->drain_lock_dl);
        break;
    case QAHW_STREAM_CBK_EVENT_ADSP:
        fprintf(stderr, "received event - QAHW_STREAM_CBK_EVENT_ADSP\n");
        if (payload != NULL) {
            fprintf(stderr, "event_type %d\n", payload[0]);
            fprintf(stderr, "param_length %d\n", payload[1]);
            for (i=2; i* sizeof(uint32_t) <= payload[1]; i++)
                fprintf(stderr, "param[%d] = 0x%x\n", i, payload[i]);
        }
        break;
    case QAHW_STREAM_CBK_EVENT_ERROR:
        fprintf(stderr, "received event - QAHW_STREAM_CBK_EVENT_ERROR\n");
        stop_dl = true;
        break;
    default:
        break;
    }
    return 0;
}

static int write_to_hal(qahw_stream_handle_t* out_handle,
                        char *data, size_t bytes, void *params_ptr)
{
    voice_stream_config *stream_params = (voice_stream_config*) params_ptr;

    ssize_t ret;
    pthread_mutex_lock(&stream_params->write_lock);
    qahw_buffer_t out_buf;

    memset(&out_buf,0, sizeof(qahw_buffer_t));
    out_buf.buffer = data;
    out_buf.size = bytes;

    ret = qahw_stream_write(out_handle, &out_buf);
    if (ret < 0) {
        fprintf(stderr, " writing data to hal failed (ret = %zd)\n", ret);
    } else if ((ret != bytes) && (!stop)) {
        fprintf(stderr, " provided bytes %zd, written bytes %d\n", bytes, ret);
        fprintf(stderr, " waiting for event write ready\n");
        pthread_cond_wait(&stream_params->write_cond, &stream_params->write_lock);
        fprintf(stderr, " out of wait for event write ready\n");
    }

    pthread_mutex_unlock(&stream_params->write_lock);
    return ret;
}

static int write_to_hal_dl(qahw_stream_handle_t* out_handle, char *data,
                           size_t bytes, void *params_ptr)
{
    voice_stream_config *stream_params = (voice_stream_config*) params_ptr;

    ssize_t ret;
    pthread_mutex_lock(&stream_params->write_lock_dl);
    qahw_buffer_t out_buf;

    memset(&out_buf,0, sizeof(qahw_buffer_t));
    out_buf.buffer = data;
    out_buf.size = bytes;

    ret = qahw_stream_write(out_handle, &out_buf);
    if (ret < 0) {
        fprintf(stderr, " writing data to hal failed (ret = %zd)\n", ret);
    } else if ((ret != bytes) && (!stop_dl)) {
        fprintf(stderr, " provided bytes %zd, written bytes %d\n", bytes, ret);
        fprintf(stderr, " waiting for event write ready\n");
        pthread_cond_wait(&stream_params->write_cond_dl, &stream_params->write_lock_dl);
        fprintf(stderr, " out of wait for event write ready\n");
    }

    pthread_mutex_unlock(&stream_params->write_lock_dl);
    return ret;
}

static void *usb_play_start(void* thread_param)
{
    Node *nodeToRemove = NULL;
    void* headBuffer = NULL;
    struct pcm *usb_plbk_pcm_hndl = NULL;
    bool isVoiceOverUsb = false;
    struct timespec end;
    struct timespec now;
    int total_bytes_write_on_usb = 0;
    voice_stream_config *params = (voice_stream_config *)thread_param;
    unsigned int cap_time = params->call_length;

    usb_plbk_pcm_hndl = get_plbk_pcm_hndl();
    if (usb_plbk_pcm_hndl != NULL) {
        isVoiceOverUsb = true;
    }

    headBuffer = calloc(1, MAX_BUFFER_SIZE);
    if (headBuffer == NULL){
        fprintf(stderr, "%s:calloc failed\n",__func__);
        goto exit;
    }
    memset(headBuffer, 0, MAX_BUFFER_SIZE);
    clock_gettime(CLOCK_MONOTONIC, &now);
    end.tv_sec = now.tv_sec + cap_time;
    end.tv_nsec = now.tv_nsec;
    while(true && !stop){
        nodeToRemove = removeFromHead(&recLinkedList);
        if (nodeToRemove == NULL) {
            fprintf(stderr, "No node To Remove\n");
            continue;
        }
        if(MAX_BUFFER_SIZE < nodeToRemove->buffer_size) {
            fprintf(stderr, "Buffer size exceeds max buffer size\n");
            goto exit;
        }

        memcpy(headBuffer, nodeToRemove->data, nodeToRemove->buffer_size);
        if (pcm_write(usb_plbk_pcm_hndl, headBuffer, nodeToRemove->buffer_size)){
            fprintf(stderr, "Error playing sample on usb device node\n");
        }
        memset(headBuffer, 0, nodeToRemove->buffer_size);
        total_bytes_write_on_usb += total_bytes_write_on_usb;
        free(nodeToRemove->data);
        free(nodeToRemove);
        if (cap_time) {
            clock_gettime(CLOCK_MONOTONIC, &now);
            if (now.tv_sec > end.tv_sec ||
                (now.tv_sec == end.tv_sec && now.tv_nsec >= end.tv_nsec)) {
                goto exit;
            }
        }
    }
exit:
    fprintf(stderr, "%s: exiting usb play thread\n", __func__);
    free(headBuffer);
    headBuffer = NULL;
    pthread_exit(0);
}


void *usb_incall_rec_start(void * thread_param) {
    uint32_t rc = 0;
    voice_stream_config *params = (voice_stream_config *)thread_param;
    qahw_module_handle_t *qahw_mod_handle = params->qahw_mod_handle;
    qahw_stream_handle_t *in_handle = NULL;
    uint32_t num_dev = 1;
    audio_devices_t in_device[1] = { AUDIO_DEVICE_IN_WIRED_HEADSET };
    struct qahw_stream_attributes attr;
    qahw_buffer_t in_buf;
    struct pcm *usb_plbk_pcm_hndl = NULL;
    bool isVoiceOverUsb = false;
    int data_sz = 0;
    ssize_t bytes_read = -1;
    char *usb_buffer = NULL;
    char *buffer_pointer =NULL;
    int input_buf_size = 0;
    unsigned int usb_buf_size = 0;
    int total_bytes_written = 0;
    char *buffer;

    if (qahw_mod_handle == NULL) {
        fprintf(stderr, "%s: qahw_load_module failed\n" ,__func__);
        pthread_exit(0);
    }

    if(stream_params.output_device[0] == AUDIO_DEVICE_OUT_SPEAKER) {
        in_device[0] = AUDIO_DEVICE_IN_BACK_MIC;
    }

    usb_plbk_pcm_hndl = get_plbk_pcm_hndl();
    if (usb_plbk_pcm_hndl != NULL) {
        isVoiceOverUsb = true;
    }

    if(params->in_call_rec) {
        switch (params->tp_dir) {
        case 0:
            attr.type = QAHW_AUDIO_CAPTURE_VOICE_CALL_RX;
            break;
        case 1:
            attr.type = QAHW_AUDIO_CAPTURE_VOICE_CALL_TX;
            break;
        case 2:
            attr.type = QAHW_AUDIO_CAPTURE_VOICE_CALL_RX_TX;
            break;
        default:
            fprintf(stderr, "%s: invalid tp direction \n", __func__);
            pthread_exit(0);
            break;
        }
            attr.attr.audio.config.sample_rate = 48000;
    }

    attr.direction = QAHW_STREAM_INPUT;
    attr.attr.audio.config.format = AUDIO_FORMAT_PCM_16_BIT;
    attr.attr.audio.config.channel_mask = 0xC;

    rc = qahw_stream_open(qahw_mod_handle,
                          attr,
                          num_dev,
                          in_device,
                          0,
                          NULL,
                          NULL,
                          NULL,
                          &(in_handle));
    if (rc) {
        fprintf(stderr, "%s: open input device failed!\n", __func__);
        pthread_exit(0);
    }

    /* Get buffer size to get upper bound on data to read from the HAL */
    size_t in_buffer_size;
    size_t out_buffer_size;
    rc = qahw_stream_get_buffer_size(in_handle, &in_buffer_size, &out_buffer_size);

    buffer = (char *)calloc(1, in_buffer_size);
    size_t written_size;
    int bps = 16;

    if (buffer == NULL) {
        fprintf(stderr, "calloc failed!!, handle(%d)\n", in_handle);
        pthread_exit(0);
    }

    if (isVoiceOverUsb) {
        usb_buf_size =  pcm_frames_to_bytes(usb_plbk_pcm_hndl, pcm_get_buffer_size(usb_plbk_pcm_hndl));
        if (in_buffer_size < usb_buf_size) {
            usb_buffer = (char *)calloc(1, in_buffer_size);
        } else {
            usb_buffer = (char *)calloc(1, usb_buf_size);
        }
        if (usb_buffer == NULL) {
            isVoiceOverUsb = false;
            free(usb_buffer);
            pcm_close(usb_plbk_pcm_hndl);
            return NULL;
        }
    }

    if (params->rec_file == NULL) {
        fprintf(stderr, "no record stream provided\n", in_handle);
        pthread_exit(0);
        return NULL;
    }

    FILE *fd = fopen(params->rec_file, "w");
    if (fd == NULL) {
        fprintf(stderr, "File open failed \n");
        free(buffer);
        pthread_exit(0);
    }
    struct wav_header hdr;
    hdr.riff_id = ID_RIFF;
    hdr.riff_sz = 0;
    hdr.riff_fmt = ID_WAVE;
    hdr.fmt_id = ID_FMT;
    hdr.fmt_sz = 16;
    hdr.audio_format = FORMAT_PCM;
    hdr.num_channels = 2;
    hdr.sample_rate = attr.attr.audio.config.sample_rate;
    hdr.byte_rate = hdr.sample_rate * hdr.num_channels * (bps / 8);
    hdr.block_align = hdr.num_channels * (bps / 8);
    hdr.bits_per_sample = bps;
    hdr.data_id = ID_DATA;
    hdr.data_sz = 0;
    fwrite(&hdr, 1, sizeof(hdr), fd);

    memset(&in_buf, 0, sizeof(qahw_buffer_t));

    while (true && !stop) {
        in_buf.buffer = buffer;
        in_buf.size = in_buffer_size;
        bytes_read = qahw_stream_read(in_handle, &in_buf);
        buffer_pointer = in_buf.buffer;
        input_buf_size = in_buf.size;

	if (isVoiceOverUsb){
            total_bytes_written = in_buffer_size;
            written_size = fwrite(in_buf.buffer, 1, in_buffer_size, fd);
            if (written_size < in_buffer_size) {
                fprintf(stderr, "Error in fwrite\n");
                break;
            }
            addToTail (&recLinkedList, in_buf.buffer, in_buffer_size);
            data_sz += in_buffer_size;
        }
    }
    /* update lengths in header */
    hdr.data_sz = data_sz;
    hdr.riff_sz = data_sz+44 - 8;
    fseek(fd, 0, SEEK_SET);
    fwrite(&hdr, 1, sizeof(hdr), fd);
    free(buffer);
    fclose(fd);
    fd = NULL;
    free(usb_buffer);
    usb_buffer = NULL;
    pcm_close(usb_plbk_pcm_hndl);

    /* Close input stream and device. */
    rc = qahw_stream_standby(in_handle);
    if (rc) {
        fprintf(stderr, "out standby failed %d, handle(%d)\n", rc, in_handle);
    }

    rc = qahw_stream_close(in_handle);
    if (rc) {
        fprintf(stderr, "could not close input stream %d, handle(%d)\n", rc, in_handle);
    }
    pthread_exit(0);
    return NULL;
}



static void* usb_rec_func(void * thread_param)
{
    voice_stream_config *params = (voice_stream_config *)thread_param;
    unsigned int input_buf_size = 0;
    int ret = 0;
    char  *data_ptr = NULL;
    int bytes_written_to_usb = 0;
    int out_bytes_wanted = 4400;
    struct pcm *usb_rec_pcm_hndl;
    thread_event_type *t_event_type = NULL;
    struct wav_header hdr;
    int total_bytes_read_on_usb = 0;
    struct timespec end;
    struct timespec now;
    unsigned int cap_time = params->call_length;
    t_event_type = (thread_event_type *)malloc(sizeof(thread_event_type));

    if (t_event_type == NULL) {
        fprintf(stderr, "failed to for t_event_type\n");
        pthread_exit(0);
    }
    usb_rec_pcm_hndl = get_rec_pcm_hndl();
    if (usb_rec_pcm_hndl == NULL) {
        fprintf(stderr, " null returned for usb_rec_pcm_hndl\n");
        pthread_exit(0);
        return NULL;
    }
    usb_data_ptr = (char *)malloc(out_bytes_wanted);
    if (usb_data_ptr == NULL) {
        fprintf(stderr, "failed to allocate usb_data_ptr\n");
        pthread_exit(0);
    }
    usb_rec_buf_size = pcm_frames_to_bytes(usb_rec_pcm_hndl,  pcm_get_buffer_size(usb_rec_pcm_hndl));
    usb_rec_buffer = (char *)calloc(1, 2*usb_rec_buf_size);
    if (usb_rec_buffer == NULL) {
        fprintf(stderr, " usb_rec_buffer calloc failed\n");
        free(usb_rec_buffer);
        pcm_close(usb_rec_pcm_hndl);
        return NULL;
    }
    if (params->usb_rec_file == NULL) {
        fprintf(stderr, "no record stream provided\n");
        pthread_exit(0);
        return NULL;
    }
    FILE *fd = fopen(params->usb_rec_file, "w");
    if (fd == NULL) {
        fprintf(stderr, "File open failed \n");
        pthread_exit(0);
    }
    hdr.riff_id = ID_RIFF;
    hdr.riff_sz = 0;
    hdr.riff_fmt = ID_WAVE;
    hdr.fmt_id = ID_FMT;
    hdr.fmt_sz = 16;
    hdr.audio_format = FORMAT_PCM;
    hdr.num_channels = 2;
    hdr.sample_rate = 48000;
    hdr.byte_rate = 96000;
    hdr.block_align = hdr.num_channels * (16 / 8);
    hdr.bits_per_sample = 16;
    hdr.data_id = ID_DATA;
    hdr.data_sz = 0;
    fwrite(&hdr, 1, sizeof(hdr), fd);
    fprintf(stderr, "file %s opened for write \n", params->usb_rec_file);

    clock_gettime(CLOCK_MONOTONIC, &now);
    end.tv_sec = now.tv_sec + cap_time;
    end.tv_nsec = now.tv_nsec;

    while (capturing && !pcm_read(usb_rec_pcm_hndl, usb_rec_buffer, usb_rec_buf_size)) {
        if (fwrite(usb_rec_buffer, 1, usb_rec_buf_size, fd) != usb_rec_buf_size) {
            fprintf(stderr,"Error capturing sample\n");
            break;
        }
        bytes_written_to_usb = write_to_hal(hal_out_handle, usb_rec_buffer, usb_rec_buf_size, params);
        fprintf(stderr, " %s:bytes_written_to_usb:%d\n", __func__,bytes_written_to_usb);
        total_bytes_read_on_usb += usb_rec_buf_size;
        if (cap_time) {
            clock_gettime(CLOCK_MONOTONIC, &now);
            if (now.tv_sec > end.tv_sec ||
               (now.tv_sec == end.tv_sec && now.tv_nsec >= end.tv_nsec)) {
                hdr.data_sz = total_bytes_read_on_usb;
                hdr.riff_sz = total_bytes_read_on_usb + 44 - 8;
                fseek(fd, 0, SEEK_SET);
                fwrite(&hdr, 1, sizeof(hdr), fd);
                fclose(fd);
                fd = NULL;
                break;
            }
       }
    }
    fprintf(stderr, " %s:total_bytes_read_on_usb:%d\n", __func__,total_bytes_read_on_usb);
    free(t_event_type);
    t_event_type = NULL;
    free(usb_rec_buffer);
    usb_rec_buffer = NULL;
    pthread_exit(0);
}

static void* usb_host_rec_func(void * thread_param) {
    voice_stream_config *params = (voice_stream_config *)thread_param;
    int out_bytes_wanted = 4400;
    struct pcm *usb_rec_pcm_hndl;
    thread_event_type *t_event_type = NULL;
    int total_bytes_read_from_usb = 0;
    struct timespec end;
    struct timespec now;
    unsigned int cap_time = params->call_length;
    t_event_type = (thread_event_type *)malloc(sizeof(thread_event_type));

    usb_rec_pcm_hndl = get_rec_pcm_hndl();
    if (usb_rec_pcm_hndl == NULL) {
        fprintf(stderr, " null returned for usb_rec_pcm_hndl\n");
        pthread_exit(0);
        return NULL;
    }
    usb_data_ptr = (char *)calloc(1, out_bytes_wanted);
    if (usb_data_ptr == NULL) {
        fprintf(stderr, "failed to allocate usb_data_ptr\n");
        pthread_exit(0);
    }
    usb_rec_buf_size = pcm_frames_to_bytes(usb_rec_pcm_hndl,  pcm_get_buffer_size(usb_rec_pcm_hndl));
    usb_rec_buffer = (char *)calloc(1, 2*usb_rec_buf_size);
    if (usb_rec_buffer == NULL) {
        fprintf(stderr, " usb_rec_buffer calloc failed\n");
        free(usb_rec_buffer);
        pcm_close(usb_rec_pcm_hndl);
        return NULL;
    }

    clock_gettime(CLOCK_MONOTONIC, &now);
    end.tv_sec = now.tv_sec + cap_time;
    end.tv_nsec = now.tv_nsec;

    while (capturing && !pcm_read(usb_rec_pcm_hndl, usb_rec_buffer, usb_rec_buf_size)) {
        total_bytes_read_from_usb += usb_rec_buf_size;
        addToTail (&playLinkedlist, usb_rec_buffer, usb_rec_buf_size);
        if (cap_time) {
            clock_gettime(CLOCK_MONOTONIC, &now);
            if (now.tv_sec > end.tv_sec ||
               (now.tv_sec == end.tv_sec && now.tv_nsec >= end.tv_nsec)) {
                cap_time = 0;
                break;
            }
       }
    }
    free(t_event_type);
    t_event_type = NULL;
    free(usb_rec_buffer);
    usb_rec_buffer = NULL;
    pthread_exit(0);
}

static void* usb_incall_play_func(void * thread_param) {
    voice_stream_config *params = (voice_stream_config *)thread_param;
    qahw_module_handle_t *qahw_mod_handle = params->qahw_mod_handle;
    qahw_stream_handle_t *out_handle = NULL;
    audio_devices_t out_device[1] = { AUDIO_DEVICE_OUT_SPEAKER };
    struct qahw_stream_attributes attr;
    struct timespec end;
    struct timespec now;
    unsigned int play_time = params->call_length;
    struct qahw_modifier_kv modifier;
    bool is_offload = false;
    struct pcm *usb_rec_pcm_hndl = NULL;
    bool isVoiceOverUsb = false;
    uint32_t rc = 0;
    uint32_t num_dev = 1;
    size_t in_bytes_wanted = 0;
    size_t out_bytes_wanted = 0;
    Node *nodeToRemove = NULL;
    char  *data_ptr = NULL;
    unsigned int total_bytes_read_from_usb = 0;
    int bytes_written_to_hal = 0;

    if (qahw_mod_handle == NULL) {
        fprintf(stderr, " qahw_load_module failed");
        pthread_exit(0);
    }

    out_device[0] = stream_params.output_device[0];
    attr.direction = QAHW_STREAM_OUTPUT;

    if(params->in_call_playback) {
        if (params->file_type == FILE_WAV ) {
            attr.attr.audio.config.sample_rate = 48000;
            attr.type = QAHW_AUDIO_PLAYBACK_VOICE_CALL_MUSIC;
            attr.attr.audio.config.format = AUDIO_FORMAT_PCM_16_BIT;
            attr.attr.audio.config.channel_mask = 0x3;
        } else if ( params->file_type == FILE_AMR_WB_PLUS ) {
            /* Currently the requirement is for AMRWB+ so hardcoding the values,
             * can be changed if more formats supported for
             * incall delivery */
            attr.attr.audio.config.format = AUDIO_FORMAT_AMR_WB_PLUS;
            attr.attr.audio.config.offload_info.sample_rate = 48000;
            attr.attr.audio.config.offload_info.format = AUDIO_FORMAT_AMR_WB_PLUS;
            attr.attr.audio.config.channel_mask = 0x3;
            attr.type = QAHW_AUDIO_COMPRESSED_PLAYBACK_VOICE_CALL_MUSIC;
            attr.attr.audio.config.offload_info.version = AUDIO_OFFLOAD_INFO_VERSION_CURRENT;
            attr.attr.audio.config.offload_info.size = sizeof(audio_offload_info_t);
            modifier.key = "music_offload_amrwbplus_bitstream_fmt";
            modifier.value = 1;
            is_offload = true;
        }
    }

    usb_rec_pcm_hndl = get_rec_pcm_hndl();
    if (usb_rec_pcm_hndl != NULL) {
        isVoiceOverUsb = true;
        fprintf(stderr, " isVoiceOverUsb usecase\n");
    }

    if (is_offload) {
        rc = qahw_stream_open(qahw_mod_handle,
                              attr,
                              num_dev,
                              out_device,
                              1,
                              &modifier,
                              async_callback,
                              params,
                              &(out_handle));
        if (rc) {
            pthread_exit(0);
        }
    } else {
        rc = qahw_stream_open(qahw_mod_handle,
                             attr,
                             num_dev,
                             out_device,
                             0,
                             NULL,
                             NULL,
                             NULL,
                             &(out_handle));
        if (rc) {
            pthread_exit(0);
        }
    }

    rc = qahw_stream_get_buffer_size(out_handle ,&in_bytes_wanted, &out_bytes_wanted);
    data_ptr = (char *)calloc(1, out_bytes_wanted);
    if (data_ptr == NULL) {
        fprintf(stderr, "failed to allocate data buffer\n");
        pthread_exit(0);
    }
    usb_rec_buf_size = pcm_frames_to_bytes(usb_rec_pcm_hndl,  pcm_get_buffer_size(usb_rec_pcm_hndl));
    usb_rec_buffer = (char *)calloc(1, 2*usb_rec_buf_size);
    if (usb_rec_buffer == NULL) {
        fprintf(stderr, " usb_rec_buffer calloc failed\n");
        free(usb_rec_buffer);
        pcm_close(usb_rec_pcm_hndl);
        return NULL;
    }
    while(true && !stop){
        nodeToRemove = removeFromHead(&playLinkedlist);
        if (nodeToRemove == NULL) {
            fprintf(stderr, "No node To Remove\n");
            continue;
        }
        memcpy(usb_rec_buffer, nodeToRemove->data, usb_rec_buf_size);
        bytes_written_to_hal = write_to_hal(out_handle, usb_rec_buffer, usb_rec_buf_size, params);
        memset(usb_rec_buffer, 0, usb_rec_buf_size);
        total_bytes_read_from_usb += usb_rec_buf_size;

        if (play_time) {
            clock_gettime(CLOCK_MONOTONIC, &now);
            if (now.tv_sec > end.tv_sec ||
               (now.tv_sec == end.tv_sec && now.tv_nsec >= end.tv_nsec)) {
                play_time = 0;
                break;
            }
       }
        free(nodeToRemove->data);
        free(nodeToRemove);
    }
    free(usb_rec_buffer);
    usb_rec_buffer = NULL;
    pthread_exit(0);
}

void usage() {
    printf(" \n Command \n");
    printf(" \n hal_voice_test <options>   - starts voice call\n");
    printf(" \n Options\n");
    printf(" -i  --vsid <vsid>                   - vsid to use sim1<11C05000> sim2<29965107>.\n");
    printf(" -d  --device <decimal value>        - see system/media/audio/include/system/audio.h for device values\n");
    printf(" -l  --length <call length>          - call length in sec.\n");
    printf(" -m  --multi_call <number of calls>  - number of calls to make.\n");
    printf(" -r  --in_call_rec <filename to record to> -t  - tp_dir <0 = DL, 1 = UL, 2 = BOTH >\n");
    printf(" -p  --in_call_playback <filename to play from>  play audio to voice call\n");
    printf(" -o  --file_type <21 > AMRWB+, 20 > AMRWB, 19 > AMR \n");
    printf(" -v  --vol <val>               - volume.\n");
    printf(" -u  --mute <dir>              - <dir 0= tx, 1 = rx> .\n");
    printf(" -c  --dtmf_gen                                     .\n");
    printf(" -y  --tty_mode                - <MODE_OFF = 0, MODE_FULL = 1, MODE_VCO  = 2, MODE_HCO = 3\n");
    printf(" -e  --in_dl_call_playback <filename to play from> play downlink audio to voice call\n");
    printf(" -n  --stream type             - <1 = QAHW_VOICECALL, 2 = QAHW_ECALL \n");
    printf(" -w  --dtmf_detect                                   .\n");
}

void stop_signal_handler(int signal __unused) {
    stop = true;
    stop_dl = true;
}

static void qti_audio_server_death_notify_cb(void *ctxt __unused) {
    fprintf(stderr, "qas died\n");
    stop = true;
    stop_dl = true;
}

void *rec_start(void *thread_param) {
    uint32_t rc = 0;
    voice_stream_config *params = (voice_stream_config *)thread_param;
    qahw_module_handle_t *qahw_mod_handle = params->qahw_mod_handle;
    qahw_stream_handle_t *in_handle = NULL;
    uint32_t num_dev = 1;
    audio_devices_t in_device[1] = { AUDIO_DEVICE_IN_WIRED_HEADSET };
    struct qahw_stream_attributes attr;
    qahw_buffer_t in_buf;
    unsigned int usb_buffer_size;
    struct pcm *usb_plbk_pcm_hndl = NULL;
    bool isVoiceOverUsb = false;
    int data_sz = 0;
    int read_usb_size = 0;
    int num_usb_read = 0;
    ssize_t bytes_read = -1;
    char *usb_buffer = NULL;
    char *buffer_pointer =NULL;
    int input_buf_size = 0;
    unsigned int usb_buf_size = 0;

    fprintf(stderr, "%s: starting rec thread\n", __func__);
    if (qahw_mod_handle == NULL) {
        fprintf(stderr, "%s: qahw_load_module failed\n" ,__func__);
        pthread_exit(0);
    }

    if(stream_params.output_device[0] == AUDIO_DEVICE_OUT_SPEAKER) {
        in_device[0] = AUDIO_DEVICE_IN_BACK_MIC;
    }

    if(params->in_call_rec) {
        fprintf(stderr, "%s: setting in call record params\n", __func__);
        switch (params->tp_dir) {
        case 0:
            attr.type = QAHW_AUDIO_CAPTURE_VOICE_CALL_RX;
            break;
        case 1:
            attr.type = QAHW_AUDIO_CAPTURE_VOICE_CALL_TX;
            break;
        case 2:
            attr.type = QAHW_AUDIO_CAPTURE_VOICE_CALL_RX_TX;
            break;
        default:
            fprintf(stderr, "%s: invalid tp direction \n", __func__);
            pthread_exit(0);
            break;
        }
            attr.attr.audio.config.sample_rate = 48000;
    }
    if(params->hpcm) {
        fprintf(stderr, "setting host pcm params\n");
        switch(params->hpcm_tp) {
            case QAHW_HPCM_TAP_POINT_RX:
                attr.type = QAHW_AUDIO_HOST_PCM_RX_RECORD;
                break;
            case QAHW_HPCM_TAP_POINT_TX:
                attr.type = QAHW_AUDIO_HOST_PCM_TX_RECORD;
                break;
            case QAHW_HPCM_TAP_POINT_RX_TX:
                attr.type = QAHW_AUDIO_HOST_PCM_TX_RX;
                break;
            default:
                fprintf(stderr, "unsupported tp %d\n", params->hpcm_tp);
                pthread_exit(0);
                break;
        }
        attr.attr.audio.config.sample_rate = params->hpcm_sr;
    }
    attr.direction = QAHW_STREAM_INPUT;
    attr.attr.audio.config.format = AUDIO_FORMAT_PCM_16_BIT;

    rc = qahw_stream_open(qahw_mod_handle,
                          attr,
                          num_dev,
                          in_device,
                          0,
                          NULL,
                          NULL,
                          NULL,
                          &(in_handle));
    if (rc) {
        fprintf(stderr, "%s: open input device failed!\n", __func__);
        pthread_exit(0);
    }

    /* Get buffer size to get upper bound on data to read from the HAL */
    size_t in_buffer_size;
    size_t out_buffer_size;
    rc = qahw_stream_get_buffer_size(in_handle, &in_buffer_size, &out_buffer_size);
    char *buffer = (char *)calloc(1, in_buffer_size);
    size_t written_size;
    int bps = 16;

    if (buffer == NULL) {
        fprintf(stderr, "calloc failed!!, handle(%d)\n", in_handle);
        pthread_exit(0);
    }

    usb_plbk_pcm_hndl = get_plbk_pcm_hndl();
    if (usb_plbk_pcm_hndl != NULL) {
        isVoiceOverUsb = true;
        usb_buf_size =  pcm_frames_to_bytes(usb_plbk_pcm_hndl, pcm_get_buffer_size(usb_plbk_pcm_hndl));
        usb_buffer = (char *)calloc(1, usb_buf_size);
        if (usb_buffer == NULL) {
            fprintf(stderr, " usb_buffer calloc failed\n");
            isVoiceOverUsb = false;
            free(buffer);
            pcm_close(usb_plbk_pcm_hndl);
            return NULL;
        }
    }


    if (params->rec_file == NULL) {
        fprintf(stderr, "no record stream provided\n", in_handle);
        pthread_exit(0);
        return NULL;
    }

    FILE *fd = fopen(params->rec_file, "w");
    if (fd == NULL) {
        fprintf(stderr, "File open failed \n");
        free(buffer);
        pthread_exit(0);
    }
    struct wav_header hdr;
    hdr.riff_id = ID_RIFF;
    hdr.riff_sz = 0;
    hdr.riff_fmt = ID_WAVE;
    hdr.fmt_id = ID_FMT;
    hdr.fmt_sz = 16;
    hdr.audio_format = FORMAT_PCM;
    hdr.num_channels = 1;
    hdr.sample_rate = attr.attr.audio.config.sample_rate;
    hdr.byte_rate = hdr.sample_rate * hdr.num_channels * (bps / 8);
    hdr.block_align = hdr.num_channels * (bps / 8);
    hdr.bits_per_sample = bps;
    hdr.data_id = ID_DATA;
    hdr.data_sz = 0;
    fwrite(&hdr, 1, sizeof(hdr), fd);

    memset(&in_buf, 0, sizeof(qahw_buffer_t));
    fprintf(stderr, "file %s opened for write \n", params->rec_file);
    if(params->hpcm){
        in_buffer_size = (params->hpcm_sr * 20 *2) /1000;
        usleep(20000);
    }
    while (true && !stop) {
        in_buf.buffer = buffer;
        in_buf.size = in_buffer_size;
        fprintf(stderr, " calling qahw_stream_read, in_buffer_size:%d \n", in_buffer_size);
        bytes_read = qahw_stream_read(in_handle, &in_buf);
        fprintf(stderr, " returned from qahw_stream_read, in_buffer_size:%d, bytes_read:%d \n", in_buffer_size, bytes_read);
        buffer_pointer = in_buf.buffer;
        input_buf_size = in_buf.size;
        if (isVoiceOverUsb && (usb_plbk_pcm_hndl != NULL)) {
            while ((buffer_pointer != NULL) && (input_buf_size > 0)) {
                read_usb_size = (usb_buf_size < input_buf_size ) ? usb_buf_size : input_buf_size;
                snprintf(usb_buffer,read_usb_size, "%s\n", buffer_pointer);
                if (pcm_write(usb_plbk_pcm_hndl, in_buf.buffer, in_buf.size)) {
                    fprintf(stderr, "Error playing sample on usb device node\n");
                    break;
                } else {
                    fprintf(stderr, "playing sample on usb device node\n");
                }
                buffer_pointer += read_usb_size;
                input_buf_size -= read_usb_size;
            }
        } else {
            written_size = fwrite(in_buf.buffer, 1, in_buffer_size, fd);
            if (written_size < in_buffer_size) {
                fprintf(stderr, "Error in fwrite\n");
                break;
            }
        }
        data_sz += in_buffer_size;
    }
    fprintf(stderr, "rec ended\n");
    /* update lengths in header */
    hdr.data_sz = data_sz;
    hdr.riff_sz = data_sz + 44 - 8;
    fseek(fd, 0, SEEK_SET);
    fwrite(&hdr, 1, sizeof(hdr), fd);
    free(buffer);
    fclose(fd);
    fd = NULL;
    fprintf(stderr, " closing input, handle(%d)", in_handle);

    /* Close input stream and device. */
    rc = qahw_stream_standby(in_handle);
    if (rc) {
        fprintf(stderr, "out standby failed %d, handle(%d)\n", rc, in_handle);
    }

    rc = qahw_stream_close(in_handle);
    if (rc) {
        fprintf(stderr, "could not close input stream %d, handle(%d)\n", rc, in_handle);
    }

    /* Print instructions to access the file.
     * Caution: Below ADL log shouldnt be altered without notifying automation APT since it used for
     * automation testing
     */
    fprintf(stderr, "\n\n ADL: The audio recording has been saved to %s. Please use adb pull to get "
            "the file and play it using audacity. The audio data has the "
            "following characteristics:\n Sample rate: %i\n Format: %d\n "
            "Num channels: %i\n\n",
            params->rec_file, attr.attr.audio.config.sample_rate, attr.attr.audio.config.format, 1);
    pthread_exit(0);

    return NULL;
}

int get_wav_header_length(FILE *file_stream) {
    int subchunk_size = 0;
    int wav_header_len = 0;

    fseek(file_stream, 16, SEEK_SET);
    if (fread(&subchunk_size, 4, 1, file_stream) != 1) {
        fprintf(stderr, "Unable to read subchunk:\n");
        exit(1);
    }
    if (subchunk_size < 16) {
        fprintf(stderr, "This is not a valid wav file \n");
    } else {
        wav_header_len = FORMAT_DESCRIPTOR_SIZE + SUBCHUNK1_SIZE(subchunk_size) + SUBCHUNK2_SIZE;
    }
    return wav_header_len;
}

void *playback_start(void *thread_param) {
    uint32_t rc = 0;
    voice_stream_config *params = (voice_stream_config *)thread_param;
    qahw_module_handle_t *qahw_mod_handle = params->qahw_mod_handle;
    qahw_stream_handle_t *out_handle = NULL;
    uint32_t num_dev = 1;
    audio_devices_t out_device[1] = { AUDIO_DEVICE_OUT_SPEAKER };
    struct qahw_stream_attributes attr;
    struct pcm *usb_rec_pcm_hndl = NULL;
    size_t in_bytes_wanted = 0;
    size_t out_bytes_wanted = 0;
    size_t write_length = 0;
    size_t bytes_remaining = 0;
    ssize_t bytes_written = 0;
    FILE *fp = NULL;
    FILE *usb_rec_fd = NULL;
    size_t bytes_read = 0;
    unsigned int total_bytes_read_on_usb = 0;
    qahw_buffer_t out_buf;
    char  *data_ptr = NULL;
    bool exit = false;
    bool read_complete_file = true;
    int wav_header_len;
    char header[WAV_HEADER_LENGTH_MAX] = { 0 };
    size_t bytes_to_read = 0;
    size_t offset = 0;
    bool is_offload = false;
    bool isVoiceOverUsb = false;
    struct qahw_modifier_kv modifier;
    unsigned int total_bytes_read = 0;
    struct wav_header file_header;
    unsigned int usb_frames = 0;
    char *usb_rec_buffer = NULL;
    int ret = 0;
    char *buffer_pointer =NULL;
    int input_buf_size = 0;
    int read_usb_size = 0;
    thread_event_type *t_event_type = NULL;
    struct timespec end;
    struct timespec now;
    unsigned int play_time = params->call_length;

    if (qahw_mod_handle == NULL) {
        fprintf(stderr, " qahw_load_module failed");
        pthread_exit(0);
    }

    out_device[0] = stream_params.output_device[0];
    attr.direction = QAHW_STREAM_OUTPUT;
    if(params->in_call_playback) {
        if (params->file_type == FILE_WAV ) {
            attr.attr.audio.config.sample_rate = 48000;
            attr.type = QAHW_AUDIO_PLAYBACK_VOICE_CALL_MUSIC;
            attr.attr.audio.config.format = AUDIO_FORMAT_PCM_16_BIT;
            attr.attr.audio.config.channel_mask = 0x3;
        } else if ( params->file_type == FILE_AMR_WB_PLUS ) {
            /* Currently the requirement is for AMRWB+ so hardcoding the values,
             * can be changed if more formats supported for
             * incall delivery */
            attr.attr.audio.config.format = AUDIO_FORMAT_AMR_WB_PLUS;
            attr.attr.audio.config.offload_info.sample_rate = 48000;
            attr.attr.audio.config.offload_info.format = AUDIO_FORMAT_AMR_WB_PLUS;
            attr.attr.audio.config.channel_mask = 0x3;
            attr.type = QAHW_AUDIO_COMPRESSED_PLAYBACK_VOICE_CALL_MUSIC;
            attr.attr.audio.config.offload_info.version = AUDIO_OFFLOAD_INFO_VERSION_CURRENT;
            attr.attr.audio.config.offload_info.size = sizeof(audio_offload_info_t);
            modifier.key = "music_offload_amrwbplus_bitstream_fmt";
            modifier.value = 1;
            is_offload = true;
        }
    }
    if(params->hpcm) {
        fprintf(stderr, "tp %d\n", params->hpcm_tp);
        switch(params->hpcm_tp) {
            case QAHW_HPCM_TAP_POINT_RX:
                attr.type = QAHW_AUDIO_HOST_PCM_RX_PLAYBACK;
                break;
            case QAHW_HPCM_TAP_POINT_TX:
                attr.type = QAHW_AUDIO_HOST_PCM_TX_PLAYBACK;
                break;
            case QAHW_HPCM_TAP_POINT_RX_TX:
                attr.type =  QAHW_AUDIO_HOST_PCM_TX_RX;
                break;
            default:
                fprintf(stderr, "unsupported tp %d\n", params->hpcm_tp);
                pthread_exit(0);
                break;
        }
        attr.attr.audio.config.sample_rate = params->hpcm_sr;
        attr.attr.audio.config.format = AUDIO_FORMAT_PCM_16_BIT;
        attr.attr.audio.config.channel_mask = 0x3;
    }
    usb_rec_pcm_hndl = get_rec_pcm_hndl();
    if (usb_rec_pcm_hndl != NULL) {
        isVoiceOverUsb = true;
    }

    if (params->playback_file != NULL) {
        fp = fopen(params->playback_file, "r");
        if (fp == NULL) {
            fprintf(stderr, "failed to open file %s\n",params->playback_file );
            pthread_exit(0);
        }
    } else {
        fprintf(stderr, "invalid playback file" );
        pthread_exit(0);
    }
    if (!params->hpcm && !isVoiceOverUsb) {
        if (params->file_type == FILE_WAV ) {
        /*
        * Read the wave header
        */
        fprintf(stderr, "Read the wave header" );
        if ((wav_header_len = get_wav_header_length(fp)) <= 0) {
            fprintf(stderr, "wav header length is invalid:%d\n", wav_header_len);
            pthread_exit(0);
        }
        fseek(fp, 0, SEEK_SET);
        rc = fread(header, wav_header_len, 1, fp);
        if (rc != 1) {
            fprintf(stderr, "Error fread failed\n");
            pthread_exit(0);
        }
        if (strncmp(header, "RIFF", 4) && strncmp(header + 8, "WAVE", 4)) {;
            fprintf(stderr, "Not a wave format\n");
            pthread_exit(0);
        }
        //memcpy (&stream_info->channels, &header[22], 2);
        memcpy(&attr.attr.audio.config.offload_info.sample_rate, &header[24], 4);
        memcpy(&attr.attr.audio.config.offload_info.bit_width, &header[34], 2);
        if (attr.attr.audio.config.offload_info.bit_width == 32)
            attr.attr.audio.config.offload_info.format = AUDIO_FORMAT_PCM_32_BIT;
        else if (attr.attr.audio.config.offload_info.bit_width == 24)
            attr.attr.audio.config.offload_info.format = AUDIO_FORMAT_PCM_24_BIT_PACKED;
        else
            attr.attr.audio.config.offload_info.format = AUDIO_FORMAT_PCM_16_BIT;
    }
    attr.attr.audio.config.sample_rate = attr.attr.audio.config.offload_info.sample_rate;
    attr.attr.audio.config.format = attr.attr.audio.config.offload_info.format;
    }


    if (is_offload) {
        rc = qahw_stream_open(qahw_mod_handle,
                              attr,
                              num_dev,
                              out_device,
                              1,
                              &modifier,
                              async_callback,
                              params,
                              &(out_handle));
        if (rc) {
            fprintf(stderr, " open output device failed!\n");
            pthread_exit(0);
        }
    } else {
        rc = qahw_stream_open(qahw_mod_handle,
                             attr,
                             num_dev,
                             out_device,
                             0,
                             NULL,
                             NULL,
                             NULL,
                             &(out_handle));
        if (rc) {
            fprintf(stderr, " open output device failed!\n");
            pthread_exit(0);
        }
    }
    rc = qahw_stream_get_buffer_size(out_handle ,&in_bytes_wanted, &out_bytes_wanted);
    fprintf(stderr, "out_bytes_wanted:%d\n", out_bytes_wanted);
    data_ptr = (char *)malloc(out_bytes_wanted);
    if (data_ptr == NULL) {
        fprintf(stderr, "failed to allocate data buffer\n");
        pthread_exit(0);
    }
    if(params->hpcm){
        out_bytes_wanted = (params->hpcm_sr * 20 *2) /1000;
        usleep(20000);
    }
    bytes_to_read = -1;
    read_complete_file = true;
    if (isVoiceOverUsb && (usb_rec_pcm_hndl != NULL)) {
        out_bytes_wanted = usb_rec_buf_size;
        hal_out_handle = out_handle;
        pthread_create(&usb_tid, NULL, usb_rec_func, thread_param);
        clock_gettime(CLOCK_MONOTONIC, &now);
        end.tv_sec = now.tv_sec + play_time;
        end.tv_nsec = now.tv_nsec;

        while (play_time) {
            clock_gettime(CLOCK_MONOTONIC, &now);
            if (now.tv_sec > end.tv_sec ||
                (now.tv_sec == end.tv_sec && now.tv_nsec >= end.tv_nsec)) {
                fprintf(stderr, "Test for VoiceOverUsb session completed.\n");
                pthread_join(usb_tid, NULL);
                pthread_cond_destroy(&usb_read_cond);
                pthread_cond_destroy(&usb_write_cond);
                pthread_mutex_destroy(&usb_lock);
                play_time = 0;
            }
        }
    } else {
        while (!exit && !stop) {
            if (!bytes_remaining) {
                fprintf(stderr, "reading bytes %zd\n", out_bytes_wanted);
                bytes_read = fread(data_ptr, 1, out_bytes_wanted, fp);
                fprintf(stderr, "read bytes %zd\n", bytes_read);
                if ((!read_complete_file && (bytes_to_read <= 0)) || (bytes_read <= 0)) {
                    fprintf(stderr, "end of file\n");
                    if (is_offload) {
                        params->drain_received = false;
                        qahw_stream_drain(out_handle, QAHW_DRAIN_ALL);
                        if(!params->drain_received) {
                            pthread_mutex_lock(&params->drain_lock);
                            pthread_cond_wait(&params->drain_cond, &params->drain_lock);
                            pthread_mutex_unlock(&params->drain_lock);
                        }
                        fprintf(stderr, "out of compress drain\n");
                    }
                    /*
                     * Caution: Below ADL log shouldnt be altered without notifying
                     * automation APT since it used for automation testing
                     */
                    fprintf(stderr, "ADL: playback completed successfully\n");
                    exit = true;
                    continue;
                } else {
                    if (!read_complete_file) {
                        bytes_to_read -= bytes_read;
                        if ((bytes_to_read > 0) && (bytes_to_read < out_bytes_wanted))
                            out_bytes_wanted = bytes_to_read;
                    }
                }
                bytes_remaining = write_length = bytes_read;
            }

            offset = write_length - bytes_remaining;
            fprintf(stderr, "writing to hal %zd bytes, offset %d, write length %zd\n",
                    bytes_remaining, offset, write_length);

            bytes_written = bytes_remaining;
            bytes_written = write_to_hal(out_handle, data_ptr+offset, bytes_remaining, params);
            if (bytes_written < 0) {
                fprintf(stderr, "write failed %d", bytes_written);
                exit = true;
                continue;
            }
            bytes_remaining -= bytes_written;
        }
    }

    fclose(fp);
    if (data_ptr)
        free(data_ptr);

    qahw_stream_close(out_handle);

    return NULL;
}

void *playback_dl_start(void *thread_param) {
    uint32_t rc = 0;
    voice_stream_config *params = (voice_stream_config *)thread_param;
    qahw_module_handle_t *qahw_mod_handle = params->qahw_mod_handle;
    qahw_stream_handle_t *out_handle = NULL;
    uint32_t num_dev = 1;
    audio_devices_t out_device[1] = { AUDIO_DEVICE_OUT_ECHO_CANCELLER };
    struct qahw_stream_attributes attr;
    size_t in_bytes_wanted = 0;
    size_t out_bytes_wanted = 0;
    size_t write_length = 0;
    size_t bytes_remaining = 0;
    ssize_t bytes_written = 0;
    FILE *fp = NULL;
    size_t bytes_read = 0;
    qahw_buffer_t out_buf;
    char  *data_ptr = NULL;
    bool exit = false;
    bool read_complete_file = true;
    int wav_header_len;
    char header[WAV_HEADER_LENGTH_MAX] = { 0 };
    size_t bytes_to_read = 0;
    size_t offset = 0;
    bool is_offload = false;
    int latency;
    struct qahw_modifier_kv modifier;
    unsigned int total_bytes_read = 0;

    if (qahw_mod_handle == NULL) {
        fprintf(stderr, " qahw_load_module failed");
        pthread_exit(0);
    }

    attr.direction = QAHW_STREAM_OUTPUT;
    if(params->in_dl_call_playback) {
        if (params->file_type == FILE_WAV ) {
            attr.attr.audio.config.sample_rate = 48000;
            attr.type = QAHW_AUDIO_PLAYBACK_VOICE_CALL_MUSIC;
            attr.attr.audio.config.format = AUDIO_FORMAT_PCM_16_BIT;
        } else if ( params->file_type == FILE_AMR_WB_PLUS ) {
            /* Currently the requirement is for AMRWB+ so hardcoding the values,
             * can be changed if more formats supported for
             * incall delivery */
            attr.attr.audio.config.format = AUDIO_FORMAT_AMR_WB_PLUS;
            attr.attr.audio.config.offload_info.sample_rate = 48000;
            attr.attr.audio.config.offload_info.format = AUDIO_FORMAT_AMR_WB_PLUS;
            attr.attr.audio.config.channel_mask = 0x3;
            attr.type = QAHW_AUDIO_COMPRESSED_PLAYBACK_VOICE_CALL_MUSIC;
            attr.attr.audio.config.offload_info.version = AUDIO_OFFLOAD_INFO_VERSION_CURRENT;
            attr.attr.audio.config.offload_info.size = sizeof(audio_offload_info_t);
            modifier.key = "music_offload_amrwbplus_bitstream_fmt";
            modifier.value = 1;
            is_offload = true;
        }
    }
    if(params->hpcm) {
        fprintf(stderr, "tp %d\n", params->hpcm_tp);
        switch(params->hpcm_tp) {
            case QAHW_HPCM_TAP_POINT_RX:
                attr.type = QAHW_AUDIO_HOST_PCM_RX;
                break;
            case QAHW_HPCM_TAP_POINT_TX:
                attr.type = QAHW_AUDIO_HOST_PCM_TX;
                break;
            default:
                fprintf(stderr, "unsupported tp %d\n", params->hpcm_tp);
                pthread_exit(0);
                break;
        }
        attr.attr.audio.config.sample_rate = params->hpcm_sr;
        attr.attr.audio.config.format = AUDIO_FORMAT_PCM_16_BIT;
    }


    if (params->playback_dl_file != NULL)
        fp = fopen(params->playback_dl_file, "r");
    if (fp == NULL) {
        fprintf(stderr, "failed to open file %s\n", params->playback_file);
        pthread_exit(0);
    }

    if (params->file_type == FILE_WAV ) {
        /*
        * Read the wave header
        */
        if ((wav_header_len = get_wav_header_length(fp)) <= 0) {
            fprintf(stderr, "wav header length is invalid:%d\n", wav_header_len);
            pthread_exit(0);
        }
        fseek(fp, 0, SEEK_SET);
        rc = fread(header, wav_header_len, 1, fp);
        if (rc != 1) {
            fprintf(stderr, "Error fread failed\n");
            pthread_exit(0);
        }
        if (strncmp(header, "RIFF", 4) && strncmp(header + 8, "WAVE", 4)) {;
            fprintf(stderr, "Not a wave format\n");
            pthread_exit(0);
        }
        //memcpy (&stream_info->channels, &header[22], 2);
        memcpy(&attr.attr.audio.config.offload_info.sample_rate, &header[24], 4);
        memcpy(&attr.attr.audio.config.offload_info.bit_width, &header[34], 2);
        if (attr.attr.audio.config.offload_info.bit_width == 32)
            attr.attr.audio.config.offload_info.format = AUDIO_FORMAT_PCM_32_BIT;
        else if (attr.attr.audio.config.offload_info.bit_width == 24)
            attr.attr.audio.config.offload_info.format = AUDIO_FORMAT_PCM_24_BIT_PACKED;
        else
            attr.attr.audio.config.offload_info.format = AUDIO_FORMAT_PCM_16_BIT;
    }
    attr.attr.audio.config.sample_rate = attr.attr.audio.config.offload_info.sample_rate;
    attr.attr.audio.config.format = attr.attr.audio.config.offload_info.format;

    if (is_offload) {
        rc = qahw_stream_open(qahw_mod_handle,
                              attr,
                              num_dev,
                              out_device,
                              1,
                              &modifier,
                              async_callback_dl,
                              params,
                              &(out_handle));
        if (rc) {
            fprintf(stderr, " open output device failed!\n");
            pthread_exit(0);
        }
    } else {
        rc = qahw_stream_open(qahw_mod_handle,
                             attr,
                             num_dev,
                             out_device,
                             0,
                             NULL,
                             NULL,
                             NULL,
                             &(out_handle));
        if (rc) {
            fprintf(stderr, " open output device failed!\n");
            pthread_exit(0);
        }
    }
    rc = qahw_stream_get_buffer_size(out_handle ,&in_bytes_wanted, &out_bytes_wanted);
    data_ptr = (char *)malloc(out_bytes_wanted);
    if (data_ptr == NULL) {
        fprintf(stderr, "failed to allocate data buffer\n");
        pthread_exit(0);
    }
    bytes_to_read = -1;
    read_complete_file = true;

    while (!exit && !stop_dl) {
        if (!bytes_remaining) {
            fprintf(stderr, "reading bytes %zd\n", out_bytes_wanted);
            bytes_read = fread(data_ptr, 1, out_bytes_wanted, fp);
            fprintf(stderr, "read bytes %zd\n", bytes_read);
            if ((!read_complete_file && (bytes_to_read <= 0)) || (bytes_read <= 0)) {
                fprintf(stderr, "end of file\n");
                if (is_offload) {
                    params->drain_received_dl = false;
                    qahw_stream_drain(out_handle, QAHW_DRAIN_ALL);
                    if(!params->drain_received_dl) {
                        pthread_mutex_lock(&params->drain_lock_dl);
                        pthread_cond_wait(&params->drain_cond_dl, &params->drain_lock_dl);
                        pthread_mutex_unlock(&params->drain_lock_dl);
                    }
                    fprintf(stderr, "out of compress drain\n");
                }
                /*
                 * Caution: Below ADL log shouldnt be altered without notifying
                 * automation APT since it used for automation testing
                 */
                fprintf(stderr, "ADL: playback completed successfully\n");
                exit = true;
                continue;
            } else {
                if (!read_complete_file) {
                    bytes_to_read -= bytes_read;
                    if ((bytes_to_read > 0) && (bytes_to_read < out_bytes_wanted))
                        out_bytes_wanted = bytes_to_read;
                }
            }
            bytes_remaining = write_length = bytes_read;
        }

        offset = write_length - bytes_remaining;
        fprintf(stderr, "writing to hal %zd bytes, offset %d, write length %zd\n",
                bytes_remaining, offset, write_length);

        bytes_written = bytes_remaining;
        bytes_written = write_to_hal_dl(out_handle, data_ptr+offset, bytes_remaining, params);
        if (bytes_written < 0) {
            fprintf(stderr, "write failed %d", bytes_written);
            exit = true;
            continue;
        }
        bytes_remaining -= bytes_written;
    }

    fclose(fp);
    if (data_ptr)
        free(data_ptr);

    qahw_stream_close(out_handle);

    return NULL;
}

int main(int argc, char *argv[]) {

    uint32_t rc = 0;
    int opt = 0;
    int option_index = 0;
    char *freq_values = NULL;
    char *freq = NULL;
    qahw_stream_direction dir;
    int call_count = 0;
    bool isVoiceOverUsb = false;
    int period_size = 0;
    int period_count = 0;
    int call_lenght = 0;
    pthread_t tid_rec;
    pthread_t tid_pb;
    pthread_t tid_dl_pb;
    pthread_t  usb_writer, usb_reader, usb_host_reader, usb_incall_writer;
    char kv[QAHW_KV_PAIR_LENGTH];

    init_stream();

    struct option long_options[] = {
        /* These options set a flag. */
        { "vsid",     required_argument,    0, 'i' },
        { "device",     required_argument,    0, 'd' },
        { "call_length",     required_argument,    0, 'l' },
        { "help",          no_argument,          0, 'h' },
        { "in_call_playback",  required_argument,  0, 'p' },
        { "in_call_rec",  required_argument,  0, 'r' },
        { "host_pcm",  no_argument,  0, 'b' },
        { "tp_dir",  required_argument,  0, 't' },
        { "file",  required_argument,  0, 'f' },
        { "hpcm_tp",  required_argument,  0, 'a' },
        { "vol",  required_argument,  0, 'v' },
        { "mute",  required_argument,  0, 'u' },
        { "tty_mode",  required_argument,  0, 'y' },
        { "dtmf_gen", no_argument,  0, 'c' },
        { "file_type",  required_argument,  0, 'o' },
        { "in_dl_call_playback",  required_argument,  0, 'e' },
        { "stream_type", no_argument,  0, 'n' },
        { "dtmf_detect", no_argument,  0, 'w' },
        { "hpcm_sr", required_argument,  0, 's' },
        { "voice_over_usb", no_argument,  0, 'g' },
        { "usb_period_size", required_argument,  0, 'j' },
        { "usb_period_count", no_argument,  0, 'k' },
        { 0, 0, 0, 0 }
    };

    while ((opt = getopt_long(argc,
                              argv,
                                "-v:d:l:m:p:r:t:f:a:b:h:i:u:y:c:w:o:e:n:s:g:j:k:",
                              long_options,
                              &option_index)) != -1) {

        fprintf(stderr, "for argument %c, value is %s\n", opt, optarg);

        switch (opt) {
        case 'i':
            stream_params.vsid = optarg;
            break;
        case 'g':
            isVoiceOverUsb = true;
            break;
        case 'j':
            period_size = atoll(optarg);
             fprintf(stderr, "period_size:%d\n", period_size);
            break;
        case 'k':
            period_count = atoll(optarg);
            fprintf(stderr, "period_count:%d\n", period_count);
            break;
        case 'd':
            stream_params.output_device[0] = atoll(optarg);
            break;
        case 'l':
            stream_params.call_length = atoll(optarg);
            break;
        case 'm':
            stream_params.multi_call = atoll(optarg);
            break;
        case 'p':
            stream_params.in_call_playback = true;
            stream_params.playback_file = optarg;
            break;
        case 'e':
            stream_params.in_dl_call_playback = true;
            stream_params.playback_dl_file = optarg;
            break;
        case 'r':
            stream_params.in_call_rec = true;
            stream_params.rec_file = optarg;
            break;
        case 'b':
            stream_params.hpcm = true;
            break;
        case 'f':
            stream_params.playback_file = optarg;
            break;
        case 't':
            stream_params.tp_dir = atoll(optarg);
            break;
        case 'a':
            stream_params.hpcm_tp = atoll(optarg);
            break;
        case 's':
            stream_params.hpcm_sr = atoll(optarg);
            break;
        case 'v':
            stream_params.vol = atof(optarg);
            break;
        case 'u':
            stream_params.mute_dir = atoll(optarg);
            stream_params.mute = true;
            break;
        case 'y':
            stream_params.tty_mode = atoll(optarg);
            break;
        case 'c':
            fprintf(stderr, "DTMF usecase, case-'c'\n");
            stream_params.dtmf = true;
            stream_params.dtmf_gen_enable = true;
            freq_values = optarg;
            break;
        case 'o':
            stream_params.file_type = atoll(optarg);
            break;
        case 'n':
            stream_params.stream_type = atoll(optarg);
            break;
        case 'w':
            fprintf(stderr, "DTMF usecase, case-'w'\n");
            stream_params.dtmf = true;
            stream_params.dtmf_detect_enable = true;
            break;
        case 'h':
        default:
            usage();
            return 0;
        }
    }
    /*making dummy voice Over USB run, */
    /*to be cleaned */
    if (isVoiceOverUsb) {
        stream_params.in_call_playback = true;
        initLinkedList(&recLinkedList);
        initLinkedList(&playLinkedlist);
        stream_params.in_call_rec = true;
        stream_params.usb_rec_file = "/data/audio/usb_rec2.wav";
        rc = usb_init(period_size, period_count);
        if (rc) {
            fprintf(stderr, "USB init failed\n");
        } else {
            fprintf(stderr, "USB init success\n");
        }
    }

    /* Register the SIGINT to close the App properly */
    if (signal(SIGINT, stop_signal_handler) == SIG_ERR)
        fprintf(stderr, "Failed to register SIGINT:%d\n", errno);

    /* Register the SIGTERM to close the App properly */
    if (signal(SIGTERM, stop_signal_handler) == SIG_ERR)
        fprintf(stderr, "Failed to register SIGTERM:%d\n", errno);

    qahw_register_qas_death_notify_cb((audio_error_callback)qti_audio_server_death_notify_cb, context);

    fprintf(stderr, "starting voice call\n");
    if ((stream_params.qahw_mod_handle = qahw_load_module(QAHW_MODULE_ID_PRIMARY)) == NULL) {
        fprintf(stderr, "failure in Loading primary HAL\n");
        goto exit;
    }
    if(stream_params.hpcm) {
        fprintf(stderr, "calling hpcm set param.\n");
        qahw_param_payload hpcm;
        hpcm.hpcm_params.state = 1;
        snprintf(kv, QAHW_KV_PAIR_LENGTH, "hpcm_cfg=1");
        fprintf(stderr, "kv set is %s \n", kv);
        rc = qahw_set_parameters(stream_params.qahw_mod_handle, kv);
    }
    if(stream_params.dtmf) {
        fprintf(stderr, "calling dtmf set param.\n");
        qahw_param_payload dtmf;
        dtmf.dtmf_state_params.state = 1;
        snprintf(kv, QAHW_KV_PAIR_LENGTH, "dtmf_cfg=1");
        fprintf(stderr, "kv set is %s \n", kv);
        rc = qahw_set_parameters(stream_params.qahw_mod_handle, kv);
    }

    struct qahw_stream_attributes attr;

    switch(stream_params.stream_type){
            case 1:
                attr.type = QAHW_VOICE_CALL;
                break;
            case 2:
                attr.type = QAHW_ECALL;
                break;
            default:
                fprintf(stderr, "unsupported stream type %d taking default for voice call\n", stream_params.stream_type);
    attr.type = QAHW_VOICE_CALL;
                break;
        }

    attr.direction = QAHW_STREAM_INPUT_OUTPUT;
    attr.attr.voice.vsid = stream_params.vsid;
    stream_params.out_voice_handle = NULL;

    fprintf(stderr, "vsid is %s device is %d \n", attr.attr.voice.vsid, stream_params.output_device[0]);
    rc = qahw_stream_open(stream_params.qahw_mod_handle,
                          attr,
                          1,
                          stream_params.output_device,
                          0,
                          NULL,
                          NULL,
                          NULL,
                          &(stream_params.out_voice_handle));
    if (rc) {
        fprintf(stderr, "Could not open output stream.\n");
        goto unload;
    }
    /*set tty mode if needed*/
    if(stream_params.tty_mode) {
        qahw_param_payload tty;
        tty.tty_mode_params.mode = stream_params.tty_mode;
        rc = qahw_stream_set_parameters(stream_params.out_voice_handle,
                                        QAHW_PARAM_TTY_MODE, &tty);
    }
    while (stream_params.multi_call) {
        call_count++;
        rc = qahw_stream_start(stream_params.out_voice_handle);

        if (rc) {
            fprintf(stderr, "Could not start voice stream.\n");
            goto close_stream;
        }
        fprintf(stderr, "started voice call %d\n", call_count);
        /*set volume */
        struct qahw_volume_data vol;
        struct qahw_channel_vol vol_pair;

        vol_pair.channel = QAHW_CHANNEL_L;
        vol_pair.vol = stream_params.vol;
        vol.num_of_channels = 1;
        vol.vol_pair = &vol_pair;

        rc = qahw_stream_set_volume(stream_params.out_voice_handle, vol);
        if(rc){
            fprintf(stderr, "set vol failed rc %d!\n", rc);
        }
        call_lenght = stream_params.call_length;
        if (stream_params.in_call_rec) {
            fprintf(stderr, "\n Create in call record thread \n");
            if (isVoiceOverUsb) {
                rc = pthread_create(&usb_reader, NULL, (void*)&usb_incall_rec_start, (void *)&stream_params);
                if (rc) {
                    fprintf(stderr, "usb_incall_rec_start thread creation failed %d\n");
                }
                usleep(50000);
                rc = pthread_create(&usb_writer, NULL, (void*)&usb_play_start, (void *)&stream_params);
                if (rc) {
                    fprintf(stderr, "usb_play_start thread creation failed %d\n");
                }
            } else {
                rc = pthread_create(&tid_rec, NULL, rec_start, (void *)&stream_params);
                if (rc) {
                    fprintf(stderr, "in call rec thread creation failed %d\n");
                }
            }
        }
        if (stream_params.in_call_playback) {
            fprintf(stderr, "\n Create incall playback thread \n");
            if (isVoiceOverUsb) {
                rc = pthread_create(&usb_host_reader, NULL, (void*)&usb_host_rec_func, (void *)&stream_params);
                if (rc) {
                    fprintf(stderr, "usb_host_start thread creation failed %d\n");
                }
                usleep(50000);
                rc = pthread_create(&usb_incall_writer, NULL, (void*)&usb_incall_play_func, (void *)&stream_params);
                if (rc) {
                    fprintf(stderr, "usb_play_start thread creation failed %d\n");
                }
            } else {
                rc = pthread_create(&tid_pb, NULL, playback_start, (void *)&stream_params);
                if (rc) {
                    fprintf(stderr, "in call playback thread creation failed %d\n");
                }
            }
        }
        if (stream_params.in_dl_call_playback) {
            fprintf(stderr, "\n Create incall dl playback thread \n");
            rc = pthread_create(&tid_dl_pb, NULL, playback_dl_start, (void *)&stream_params);
            if (rc) {
                fprintf(stderr, "in call playback thread creation failed %d\n");
            }
        }
        if(stream_params.mute) {
           struct qahw_mute_data mute;
           mute.enable = true;
           mute.direction = stream_params.mute_dir;
           rc = qahw_stream_set_mute(stream_params.out_voice_handle, mute);
        }
        if(stream_params.dtmf_gen_enable) {
            qahw_param_payload dtmf;
            char *s = strtok_r(freq_values, ",", &freq);
            char *s1 = strtok_r(NULL, ",", &freq);
            if(!s || !s1) {
                fprintf(stderr, "invalid dtmf gen params\n");
                goto skip_dtmf_gen;
            }
            dtmf.dtmf_gen_params.low_freq = atoll(s);
            dtmf.dtmf_gen_params.high_freq = atoll(s1);
            dtmf.dtmf_gen_params.gain = stream_params.dtmf_gain;
            dtmf.dtmf_gen_params.enable = true;
            rc = qahw_stream_set_parameters(stream_params.out_voice_handle,
                                            QAHW_PARAM_DTMF_GEN, &dtmf);
            /*let play for 50 ms*/
            usleep(50000000);
            dtmf.dtmf_gen_params.enable = false;
            rc = qahw_stream_set_parameters(stream_params.out_voice_handle,
                                            QAHW_PARAM_DTMF_GEN, &dtmf);
        }
skip_dtmf_gen:
        if(stream_params.dtmf_detect_enable) {
            qahw_param_payload dtmf_det;
            dtmf_det.dtmf_detect_params.enable = 1;
            dtmf_det.dtmf_detect_params.dir = stream_params.tp_dir;
            fprintf(stderr, "dtmf detect, enable=%d, dir=%d\n",
                dtmf_det.dtmf_detect_params.enable, dtmf_det.dtmf_detect_params.dir);
            rc = qahw_stream_set_parameters(stream_params.out_voice_handle,
                                            QAHW_PARAM_DTMF_DETECT, &dtmf_det);
            usleep(50000000);
            dtmf_det.dtmf_detect_params.enable = 0;
            rc = qahw_stream_set_parameters(stream_params.out_voice_handle,
                                            QAHW_PARAM_DTMF_DETECT, &dtmf_det);
        }
        /*setup hpcm if needed*/
        if(stream_params.hpcm && stream_params.hpcm_tp) {
            fprintf(stderr, "creating hpcm threads.\n");
            switch(stream_params.tp_dir) {
                case QAHW_HPCM_DIRECTION_OUT:
                    fprintf(stderr, "\n Case:QAHW_HPCM_DIRECTION_OUT");
                    fprintf(stderr, "\n Create  hpcm playback thread \n");
                    rc = pthread_create(&tid_pb, NULL, playback_start,
                                        (void *)&stream_params);
                    break;
                case QAHW_HPCM_DIRECTION_IN:
                    fprintf(stderr, "\n Case:QAHW_HPCM_DIRECTION_IN");
                    fprintf(stderr, "\n Create  hpcm record thread \n");
                    rc = pthread_create(&tid_rec, NULL, rec_start,
                                        (void *)&stream_params);
                    break;
                case QAHW_HPCM_DIRECTION_OUT_IN:
                    fprintf(stderr, "\n Case:QAHW_HPCM_DIRECTION_OUT_IN");
                    fprintf(stderr, "\n Create hpcm record thread \n");
                    rc = pthread_create(&tid_rec, NULL, rec_start,
                                        (void *)&stream_params);
                    fprintf(stderr, "\n Create  hpcm playback thread \n");
                    rc = pthread_create(&tid_pb, NULL, playback_start,
                                        (void *)&stream_params);
                    break;
                default:
                    fprintf(stderr, "\n invalid HPCM direction  \n");
                    break;
            }
        }
        while (call_lenght) {
            usleep(1000000);
            call_lenght--;
        }
        stop = true;
        stop_dl = true;
        fprintf(stderr, "stoping call %d\n", call_count);
        rc = qahw_stream_stop(stream_params.out_voice_handle);
        stream_params.multi_call--;
        pthread_join(usb_reader, NULL);
        pthread_join(usb_writer, NULL);
        pthread_join(usb_host_reader, NULL);
        pthread_join(usb_host_reader, NULL);
        /*let session stop*/
        usleep(100000);
    }

 close_stream:
    fprintf(stderr, "closing voice stream\n");
    rc = qahw_stream_close(stream_params.out_voice_handle);

 unload:
    fprintf(stderr, "unloading hal\n");
    if (qahw_unload_module(stream_params.qahw_mod_handle) < 0) {
        fprintf(stderr, "failure in Un Loading primary HAL\n");
        return -1;
    }
    fprintf(stderr, "voice test ended\n");
 exit:
    deinit_streams();
    return 0;
}
