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
        /* UUID :{ea6a73b9-b071-4d31-88f3-2fa92918c8d5} */
        {0xea6a73b9,
         0xb071,
         0x4d31,
         {0x88, 0xf3, 0x2f, 0xa9, 0x29, 0x18, 0xc8, 0xd5}},

        /* optional configuration options here */
        {
                TRUSTY_APP_CONFIG_MIN_HEAP_SIZE(4096),
        },
};
