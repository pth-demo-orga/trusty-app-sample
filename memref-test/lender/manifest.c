/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include <lender.h>
#include <stddef.h>
#include <stdio.h>
#include <trusty_app_manifest.h>
#include <trusty_ipc.h>

trusty_app_manifest_t TRUSTY_APP_MANIFEST_ATTRS trusty_app_manifest = {
        /* UUID : {44d201b7-2132-4756-82ba-1289f63f7c47} */
        {0x44d201b7,
         0x2132,
         0x4756,
         {0x82, 0xba, 0x12, 0x89, 0xf6, 0x3f, 0x7c, 0x47}},

        /* optional configuration options here */
        {TRUSTY_APP_CONFIG_MIN_STACK_SIZE(2 * 4096),
         TRUSTY_APP_CONFIG_MIN_HEAP_SIZE(4 * 4096)},
};

/* Start the application on connection */
TRUSTY_APP_START_PORT(LENDER_PORT, IPC_PORT_ALLOW_TA_CONNECT);
