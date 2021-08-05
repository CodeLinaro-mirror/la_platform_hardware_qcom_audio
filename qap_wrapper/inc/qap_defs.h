 /*
  * Copyright (c) 2017,2020 The Linux Foundation. All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without
  * modification, are permitted provided that the following conditions are
  * met:
  *  * Redistributions of source code must retain the above copyright
  *    notice, this list of conditions and the following disclaimer.
  *  * Redistributions in binary form must reproduce the above
  *    copyright notice, this list of conditions and the following
  *    disclaimer in the documentation and/or other materials provided
  *    with the distribution.
  *  * Neither the name of The Linux Foundation nor the names of its
  *    contributors may be used to endorse or promote products derived
  *    from this software without specific prior written permission.
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

#ifndef QAP_DEFS_H
#define QAP_DEFS_H

__BEGIN_DECLS

#define MAX_SUPPORTED_OUTPUTS 4
#define DTS_MAX_CHANNELS (8)
#define MAX_SUPPORTED_PP_CONFIGS 2
#define QAP_AUDIO_MAX_CHANNELS 32

/* PCM Channel map */
typedef enum {
    /* Front left channel. */
    QAP_AUDIO_PCM_CHANNEL_L = 1,
    /* Front right channel. */
    QAP_AUDIO_PCM_CHANNEL_R = 2,
    /* Front center channel. */
    QAP_AUDIO_PCM_CHANNEL_C = 3,
    /* Left surround channel.*/
    QAP_AUDIO_PCM_CHANNEL_LS = 4,
    /* Right surround channel.*/
    QAP_AUDIO_PCM_CHANNEL_RS = 5,
    /* Low frequency effect channel. */
    QAP_AUDIO_PCM_CHANNEL_LFE = 6,
    /* Center surround channel; rear center channel. */
    QAP_AUDIO_PCM_CHANNEL_CS = 7,
    /* Center back channel. */
    QAP_AUDIO_PCM_CHANNEL_CB = QAP_AUDIO_PCM_CHANNEL_CS,
    /* Left back channel; rear left channel. */
    QAP_AUDIO_PCM_CHANNEL_LB = 8,
    /* Right back channel; rear right channel. */
    QAP_AUDIO_PCM_CHANNEL_RB = 9,
    /* Top surround channel.*/
    QAP_AUDIO_PCM_CHANNEL_TS = 10,
    /* Center vertical height channel.*/
    QAP_AUDIO_PCM_CHANNEL_CVH = 11,
    /* Top front center channel.*/
    QAP_AUDIO_PCM_CHANNEL_TFC = QAP_AUDIO_PCM_CHANNEL_CVH,
    /* Mono surround channel.*/
    QAP_AUDIO_PCM_CHANNEL_MS = 12,
    /* Front left of center channel. */
    QAP_AUDIO_PCM_CHANNEL_FLC = 13,
    /* Front right of center channel. */
    QAP_AUDIO_PCM_CHANNEL_FRC = 14,
    /* Rear left of center channel. */
    QAP_AUDIO_PCM_CHANNEL_RLC = 15,
    /* Rear right of center channel. */
    QAP_AUDIO_PCM_CHANNEL_RRC = 16,
    /* Secondary low frequency effect channel. */
    QAP_AUDIO_PCM_CHANNEL_LFE2 = 17,
    /* Side left channel. */
    QAP_AUDIO_PCM_CHANNEL_SL = 18,
    /* Side right channel. */
    QAP_AUDIO_PCM_CHANNEL_SR = 19,
    /* Top front left channel. */
    QAP_AUDIO_PCM_CHANNEL_TFL = 20,
    /* Left vertical height channel. */
    QAP_AUDIO_PCM_CHANNEL_LVH = QAP_AUDIO_PCM_CHANNEL_TFL,
    /* Top front right channel. */
    QAP_AUDIO_PCM_CHANNEL_TFR = 21,
    /* Right vertical height channel. */
    QAP_AUDIO_PCM_CHANNEL_RVH = QAP_AUDIO_PCM_CHANNEL_TFR,
    /* Top center channel. */
    QAP_AUDIO_PCM_CHANNEL_TC = 22,
    /* Top back left channel. */
    QAP_AUDIO_PCM_CHANNEL_TBL = 23,
    /* Top back right channel. */
    QAP_AUDIO_PCM_CHANNEL_TBR = 24,
    /* Top side left channel. */
    QAP_AUDIO_PCM_CHANNEL_TSL = 25,
    /* Top side right channel. */
    QAP_AUDIO_PCM_CHANNEL_TSR = 26,
    /* Top back center channel. */
    QAP_AUDIO_PCM_CHANNEL_TBC = 27,
    /* Bottom front center channel. */
    QAP_AUDIO_PCM_CHANNEL_BFC = 28,
    /* Bottom front left channel. */
    QAP_AUDIO_PCM_CHANNEL_BFL = 29,
    /* Bottom front right channel. */
    QAP_AUDIO_PCM_CHANNEL_BFR = 30,
    /* Left wide channel. */
    QAP_AUDIO_PCM_CHANNEL_LW = 31,
    /* Right wide channel. */
    QAP_AUDIO_PCM_CHANNEL_RW = 32,
    /* Left side direct channel. */
    QAP_AUDIO_PCM_CHANNEL_LSD = 33,
    /* Right side direct channel. */
    QAP_AUDIO_PCM_CHANNEL_RSD = 34
} qap_pcm_chmap;

/* FLAG to be set for input buffers from Client */
typedef enum {
    QAP_BUFFER_NO_TSTAMP,       /* Specifies there's no timestamp for the buffer */
    QAP_BUFFER_EOS,             /* Specifies End of Stream buffer */
    QAP_BUFFER_TSTAMP,          /* Specifies that timestamp is set for this input buffer */
    QAP_BUFFER_TSTAMP_CONTINUE, /* Timestamp is to be continued from previous buffer */
} qap_buffer_flags_t;

/**
  @name Channel names

  This set of @#defines is the list of the potentially available output channels.
  Use them as a bit mask to identify individual channels in the current channel
  configuration which you will find in the field output_channel_config of the
  streaminfo_t struct.
*/
/** @{ */
#define CH_FRONT_CENTER     0x0001
#define CH_FRONT_LEFT       0x0002
#define CH_FRONT_RIGHT      0x0004
#define CH_FRONT_WIDE_LEFT  0x0008
#define CH_FRONT_WIDE_RIGHT 0x0010
#define CH_SIDE_LEFT        0x0020
#define CH_SIDE_RIGHT       0x0040
#define CH_BACK_LEFT        0x0080
#define CH_BACK_RIGHT       0x0100
#define CH_BACK_CENTER      0x0200
#define CH_LFE              0x0400
/** @} */

/* Audio formats */
typedef enum {
    QAP_AUDIO_FORMAT_PCM_16_BIT,
    QAP_AUDIO_FORMAT_PCM_8_24_BIT,
    QAP_AUDIO_FORMAT_PCM_24_BIT_PACKED,
    QAP_AUDIO_FORMAT_PCM_32_BIT,
    QAP_AUDIO_FORMAT_AC3,
    QAP_AUDIO_FORMAT_AC4,
    QAP_AUDIO_FORMAT_EAC3,
    QAP_AUDIO_FORMAT_AAC,
    QAP_AUDIO_FORMAT_AAC_ADTS,
    QAP_AUDIO_FORMAT_MP2,
    QAP_AUDIO_FORMAT_MP3,
    QAP_AUDIO_FORMAT_FLAC,
    QAP_AUDIO_FORMAT_ALAC,
    QAP_AUDIO_FORMAT_APE,
    QAP_AUDIO_FORMAT_DTS,
    QAP_AUDIO_FORMAT_DTS_HD,
} qap_audio_format_t;

/* acmod values */
typedef enum {
    QAP_ACMOD_ZERO,
    QAP_ACMOD_ONE,
    QAP_ACMOD_TWO,
    QAP_ACMOD_THREE,
    QAP_ACMOD_FOUR,
    QAP_ACMOD_FIVE,
    QAP_ACMOD_SIX,
    QAP_ACMOD_SEVEN,
    QAP_ACMOD_EIGHT,
    QAP_ACMOD_NINE,
    QAP_ACMOD_TEN,
    QAP_ACMOD_ELEVEN,
    QAP_ACMOD_TWELVE,
    QAP_ACMOD_THIRTEEN,
    QAP_ACMOD_FOURTEEN,
    QAP_ACMOD_FIFTEEN,
    QAP_ACMOD_SIXTEEN,
    QAP_ACM0D_SEVENTEEN,
    QAP_ACMOD_EIGHTEEN,
    QAP_ACMOD_NINETEEN,
    QAP_ACMOD_TWENTY,
    QAP_ACMOD_TWENTY_ONE,
} qap_acmod_values_t;

/* AAC profiles */
typedef enum {
    QAP_PROFILE_AAC_MAIN = 0,
    QAP_PROFILE_AAC_LOW_COMPLEXITY,
    QAP_PROFILE_AAC_SSR,
} qap_audio_aac_profile_t;

/* DTS profiles */
typedef enum {
    QAP_PROFILE_UNKNOWN = 0,
    QAP_PROFILE_DTS_LEGACY,
    QAP_PROFILE_DTS_ES_MATRIX,
    QAP_PROFILE_DTS_ES_DISCRETE,
    QAP_PROFILE_DTS_9624,
    QAP_PROFILE_DTS_ES_8CH_DISCRETE,
    QAP_PROFILE_DTS_HIRES,
    QAP_PROFILE_DTS_MA,
    QAP_PROFILE_DTS_LBR,
    QAP_PROFILE_DTS_LOSSLESS,
} qap_audio_dts_profile_t;

/* Types of usecases - Used for graph creation of modules */
typedef enum {
    QAP_SESSION_BROADCAST,
    QAP_SESSION_DECODE_ONLY,
    QAP_SESSION_ENCODE_ONLY,
    QAP_SESSION_MS12_OTT
} qap_session_t;

/* Module FLAGS */
typedef enum {
    QAP_MODULE_FLAG_PRIMARY = 0x1,       /* Specifies Primary Decoder */
    QAP_MODULE_FLAG_SECONDARY = 0x2,     /* Specifies Secondary Decider */
    QAP_MODULE_FLAG_APP_SOUND = 0x4,     /* Specifies Appln sound */
    QAP_MODULE_FLAG_SYSTEM_SOUND = 0x5,  /* Specifies System sound */
    QAP_MODULE_FLAG_EXTERN_PCM = 0x6,     /* Specifies Secondary Decider */
    QAP_MODULE_FLAG_OTT_SOUND = 0x7      /* Specifies OTT System sound */
} qap_module_flags_t;

/* Module Types */
typedef enum {
    QAP_MODULE_DECODER = 0,       /* Specifies current module is a Decoder */
    QAP_MODULE_ENCODER,           /* Specifies current module is a Encoder */
    QAP_MODULE_MAX,
} qap_module_type_t;

/* Buffer Metadata Types */
typedef enum {
    QAP_BUFFER_SEC_METADATA_PARAMS = 0, /* Specifies secondary metadata for BD usecases*/
    QAP_BUFFER_MAX,
} qap_buffer_metadata_type_t;

/* logging levels for log handler */
typedef enum {
    QAP_LOG_ERROR,
    QAP_LOG_INFO,
    QAP_LOG_DEBUG,
} qap_log_level_t;

/* Input module config
 *   -- Called using QAP_MODULE_CMD_SET_CONFIG */
typedef struct qap_module_config {
    /* Common params */
    qap_audio_format_t format;
    int32_t sample_rate;
    int channels;
    int bit_width;
    bool is_interleaved;
    qap_module_type_t module_type; /* Specify Module Type */
    qap_module_flags_t flags;      /* Flag indicating Module's attributes */

    /* Decoder specific params */
    int32_t config_size;           /* size of module specific params */
    void *config;                  /* Decoder specific config params */
    /*TBD: Define the module specific params for all the modules */
} qap_module_config_t;

/* Session Output module config
 *   -- Called using QAP_SESSION_CMD_SET_OUTPUTS */
typedef struct qap_output_config {
    qap_audio_format_t format;
    int32_t sample_rate;
    int channels;
    uint8_t ch_map[QAP_AUDIO_MAX_CHANNELS];
    int bit_width;
    bool is_interleaved;
    uint32_t id;          /* Unique Output ID */
} qap_output_config_t;

/* Session Output module config
 *   -- Called using QAP_SESSION_CMD_SET_OUTPUTS */
typedef struct qap_session_outputs_config {
    uint32_t num_output;  /* Number of outputs needed from Module */
    qap_output_config_t output_config[MAX_SUPPORTED_OUTPUTS]; /* Maintain the active output configuration and updated after configuration done */
    qap_output_config_t update_output_config[MAX_SUPPORTED_OUTPUTS]; /* Maintain the user requested output configuration */
} qap_session_outputs_config_t;

typedef struct qap_buffer_common {
    uint32_t offset;           /* Offset to the first data sample in the buffer */
    uint32_t size;             /* Buffer Size */
    void*   data;             /* Buffer  pointer */
    int64_t timestamp;        /* Inband timestamp for input/output buffer */
} qap_buffer_common_t;

typedef struct qap_io_frames_tracker {
    uint64_t consumed_frames;       /* To track input frame count of decoder */
    uint64_t decoded_frames;         /* To track output frame count of decoder */
}qap_report_frames_t;

typedef struct qap_input_buff_params {
    qap_buffer_flags_t flags; /* Flags indicating the buffer attributes */
} qap_input_buff_params_t;

typedef struct qap_output_buff_params {
    uint32_t output_id;       /* Unique Output ID */
    void*   metadata;         /* Pointer to metadata. */
                              /* First entry in the metadata specifies type of metadata */
                              /* Possible values : qap_buffer_metadata_type_t */
   qap_output_config_t output_config;
} qap_output_buff_params_t;

/* Common structure for both input and output buffers */
typedef struct qap_audio_buffer {
    qap_buffer_common_t common_params;
    union {
        qap_input_buff_params_t  input_buf_params;
        qap_output_buff_params_t output_buf_params;
    } buffer_parms;
} qap_audio_buffer_t;

typedef enum {
/*    QAP_DECODE_SUCCESS,
    QAP_DECODE_ERROR,*/
    QAP_STATUS_ERR = -1,
    QAP_STATUS_OK = 0,
} qap_status_t;

/* PP config
 *   -- Called using QAP_SESSION_CMD_SET_PP_CONFIG */
typedef struct qap_pp_config {
    uint32_t id;         /* Unique Output ID */
    void *pp_type;
} qap_pp_config_t;

/* PP config
 *   -- Called using QAP_SESSION_CMD_SET_PP_OUTPUTS */
typedef struct {
    uint32_t num_confs;
    qap_pp_config_t pp_config[MAX_SUPPORTED_PP_CONFIGS];
} qap_session_pp_configs_t;

/* Payload Structure associated with QAP_MODULE_CALLBACK_EVENT_SEND_INPUT_BUFFER event.*/
typedef struct {
    uint32_t bytes_available;
} qap_send_buffer_t;

/* Payload Structure associated with QAP_MODULE_CALLBACK_EVENT_INPUT_CFG_CHANGE event.*/
typedef struct {
    uint8_t format;
    uint8_t ch_map[QAP_AUDIO_MAX_CHANNELS];
    uint32_t sample_rate;
    uint32_t channels;
    uint32_t bit_width;
    uint32_t profile;
} qap_input_config_t;

/* Payload Structure associated with QAP_CALLBACK_EVENT_DELAY event.
 * Delay event is raised for each output data buffer per device.
 * These delay values are with respect to dominating main input.
 * All these delay values are in samples. Even for bit-stream output.
 * */
typedef struct qap_output_delay {
    uint32_t output_id;
    /* Unique Output ID */

    uint32_t algo_delay;
    /* algo_delay is the static delay of module.
     * This is constant for a main input.*/

    uint32_t buffering_delay;
    /* buffering_delay is the dynamic delay due to internally buffered data.
     * This signified the number of samples stuck in the chain after generating output.
     * So in the next output buffer, these samples comes out first before the next frame is processed.
     * This is mostly zero if decoding frame size and rendering frame size is same. (in case of DDP 48K decoding)
     * And keeps on changing per output buffer when decoding and rendering frame size is different (in case of AAC decoding) */

    uint32_t non_main_data_offset;
    /* sample offset of the non-main data in the output buffer.
     * It means that for this sample offset, main data is not present in the output buffer.
     * This can be either silence data or PCM (system sound, OTT) when no main input data present.
     */

    uint32_t non_main_data_length;
    /* sample length of the non-main data in the output buffer. */
} qap_output_delay_t;

typedef enum {
    QAP_CALLBACK_EVENT_DATA,              /* Event to notify DATA availabilty to Client */
    QAP_CALLBACK_EVENT_OUTPUT_CFG_CHANGE, /* Event to notify client about output config change */
    QAP_CALLBACK_EVENT_EOS,               /* Event to notify Main Stream EOS */
    QAP_CALLBACK_EVENT_MAIN_2_EOS,        /* Event to notify Second Main Stream EOS */
    QAP_CALLBACK_EVENT_EOS_ASSOC,         /* Event to notify EOS of Assoc stream*/
    QAP_CALLBACK_EVENT_ERROR,             /* Event to notify ERROR */
    QAP_CALLBACK_EVENT_SUCCESS,           /* Event to notify SUCCESS */
    QAP_CALLBACK_EVENT_METADATA,          /* Event to notify METADATA availability */
    QAP_CALLBACK_EVENT_DELAY,             /* Event to notify module's delay */
} qap_callback_event_t;

typedef enum {
    QAP_MODULE_CALLBACK_EVENT_SEND_INPUT_BUFFER, /* Event to notify that a new input buffer can be sent */
    QAP_MODULE_CALLBACK_EVENT_INPUT_CFG_CHANGE,  /* Event to notify client about stream format change */
    QAP_MODULE_CALLBACK_EVENT_MAX,
} qap_module_callback_event_t;

typedef enum {
    QAP_MODULE_CMD_START,                       /* Module start command */
    QAP_MODULE_CMD_PAUSE,                       /* Module Pause command */
    QAP_MODULE_CMD_FLUSH,                       /* Module Flush command */
    QAP_MODULE_CMD_STOP,                        /* Module Stop command */
    QAP_MODULE_CMD_GET_PARAM,                   /* Command to retrieve run-time module params */
    QAP_MODULE_CMD_SET_PARAM,                   /* Command to set run-time module params */
    QAP_MODULE_CMD_SET_KVPAIRS,                 /* Command to set kvpairs to the module */
    QAP_MODULE_CMD_FIRST_PROP = 0x10000,        /* first prop command code */
} qap_module_cmd_t;

typedef enum {
    QAP_SESSION_CMD_GET_CONFIG,                  /* Get current session ouput config */
    QAP_SESSION_CMD_SET_CONFIG,                  /* Set initial session output config */
    QAP_SESSION_CMD_SET_OUTPUTS,                 /* Set run-time output config */
    QAP_SESSION_CMD_GET_PARAM,                   /* Command to retrieve run-time session params */
    QAP_SESSION_CMD_SET_PARAM,                   /* Command to set run-time session params */
    QAP_SESSION_CMD_SET_KVPAIRS,                 /* Command to set kvpairs to the session */
    QAP_SESSION_CMD_SET_PP_CONFIG,               /* Command to indicate on which id Dap processing output to be sent */
    QAP_SESSION_CMD_SET_PP_OUTPUTS,              /* Command to set multiple pp configs */
    QAP_SESSION_CMD_FIRST_PROP = 0x10000,        /* first prop command code */
} qap_session_cmd_t;

typedef void* qap_session_handle_t;
typedef void* qap_module_handle_t;
typedef void* qap_lib_handle_t;

__END_DECLS

#endif //QAP_DEFS_H
