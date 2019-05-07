/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include <stddef.h>
#include <stdio.h>
#include <trusty_app_manifest.h>

trusty_app_manifest_t TRUSTY_APP_MANIFEST_ATTRS trusty_app_manifest = {
        /* UUID : {31c9af88-c894-4c49-b726-bd4622a92b9c} */
        {0x31c9af88,
         0xc894,
         0xb726,
         {0xb7, 0x26, 0xbd, 0x46, 0x22, 0xa9, 0x2b, 0x9c}},

        /* optional configuration options here */
        {
                TRUSTY_APP_CONFIG_MIN_STACK_SIZE(1 * 4096),
                /* four pages for heap */
                TRUSTY_APP_CONFIG_MIN_HEAP_SIZE(4 * 4096),
        },
};
