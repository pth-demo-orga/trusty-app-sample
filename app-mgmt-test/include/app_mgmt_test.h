/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef APP_MGMT_H
#define APP_MGMT_H

#include <stdio.h>
#include <trusty_ipc.h>
#include <trusty_log.h>

#define BOOT_START_PORT "com.android.trusty.appmgmt.bootstartsrv"
#define NEVER_START_PORT "com.android.trusty.appmgmt.neverstartsrv"
#define RESTART_PORT "com.android.trusty.appmgmt.restartsrv"
#define START_PORT "com.android.trusty.appmgmt.portstartsrv"
#define CTRL_PORT "com.android.trusty.appmgmt.portstartsrv.ctrl"
#define SHUTDOWN_PORT "com.android.trusty.appmgmt.portstartsrv.shutdown"
#define LOADABLE_PORT "com.android.trusty.appmgmt.loadable"
#define LOADABLE_START_PORT "com.android.trusty.appmgmt.loadable.start"

#define MAX_CMD_LEN 1

#define LOAD_START_UUID                                           \
    {                                                             \
        0x979f88fb, 0x2928, 0x4295,                               \
                {0xb2, 0x74, 0x97, 0x59, 0x5a, 0xb5, 0x34, 0x9e}, \
    }

#define PORT_START_UUID                                           \
    {                                                             \
        0xc14672f9, 0x6a19, 0x4e3e,                               \
                {0x9b, 0x27, 0x0c, 0xd6, 0x02, 0x4d, 0xe4, 0x01}, \
    }

#define BUILTIN_UUID                                              \
    {                                                             \
        0x0d5471ec, 0x2113, 0x4320,                               \
                {0x80, 0x7f, 0xfa, 0x95, 0xdc, 0x60, 0x89, 0x9f}, \
    }

enum {
    CMD_NOP = 0,
    CMD_CLOSE_PORT = 1,
    CMD_EXIT = 2,
    CMD_OPEN_PORT = 3,
};

enum {
    RSP_OK = 0,
    RSP_CMD_FAILED = 1,
    RSP_INVALID_CMD = 2,
};

#endif
