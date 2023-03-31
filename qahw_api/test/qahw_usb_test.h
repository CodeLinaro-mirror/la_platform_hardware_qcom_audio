/*
** Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
** SPDX-License-Identifier: BSD-3-Clause-Clear
**/

#ifndef QAHW_USB_TEST_H
#define QAHW_USB_TEST_H
#endif

#include <getopt.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <cutils/str_parms.h>
#include <tinyalsa/asoundlib.h>
#include "qahw_api.h"
#include "qahw_defs.h"

extern int first_usb_read_done;
extern int first_usb_write_done;
extern char *usb_buffer;
extern unsigned int usb_buf_size;
int usb_init(int period_size, int period_count);
int usb_read();
int usb_write();
struct pcm * get_plbk_pcm_hndl();
struct pcm * get_rec_pcm_hndl();
