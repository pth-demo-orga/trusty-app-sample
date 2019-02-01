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
        /* UUID :{c5f297ed-f197-47ae-a9fc-f5b29f59dcb8} */
        {0xc5f297ed,
         0xf197,
         0x47ae,
         {0xa9, 0xfc, 0xf5, 0xb2, 0x9f, 0x59, 0xdc, 0xb8}},

        /* optional configuration options here */
        {
                TRUSTY_APP_CONFIG_MIN_HEAP_SIZE(4096),
                TRUSTY_APP_CONFIG_MGMT_FLAGS(
                        TRUSTY_APP_MGMT_FLAGS_DEFERRED_START |
                        TRUSTY_APP_MGMT_FLAGS_RESTART_ON_EXIT),
        },
};
