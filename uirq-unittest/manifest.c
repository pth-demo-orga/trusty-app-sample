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
        /* UUID : {e20af937-a4d0-4b95-b852-95ef21333cd1} */
        {0xe20af937,
         0xa4d0,
         0x4b95,
         {0xb8, 0x52, 0x95, 0xef, 0x21, 0x33, 0x3c, 0xd1}},

        /* optional configuration options here */
        {
                /* four pages for heap */
                TRUSTY_APP_CONFIG_MIN_HEAP_SIZE(1 * 4096),
        },
};
