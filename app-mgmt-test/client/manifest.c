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
        /* UUID :{eb0da59c-6a78-45c0-9870-77f13935f2a9} */
        {0x0eb0da59c,
         0x6a78,
         0x45c0,
         {0x98, 0x70, 0x77, 0xf1, 0x39, 0x35, 0xf2, 0xa9}},

        /* optional configuration options here */
        {
                TRUSTY_APP_CONFIG_MIN_HEAP_SIZE(4096),
        },
};
