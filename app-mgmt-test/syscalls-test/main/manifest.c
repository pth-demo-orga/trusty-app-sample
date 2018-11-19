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

#include <trusty_app_manifest.h>

trusty_app_manifest_t TRUSTY_APP_MANIFEST_ATTRS trusty_app_manifest = {
        /* UUID : {c51f3873-7aec-447d-96ef-a59792571b17} */
        {0xc51f3873,
         0x7aec,
         0x447d,
         {0x96, 0xef, 0xa5, 0x97, 0x92, 0x57, 0x1b, 0x17}},

        /* optional configuration options here */
        {
                TRUSTY_APP_CONFIG_MIN_HEAP_SIZE(1 * 4096),
                TRUSTY_APP_CONFIG_MIN_STACK_SIZE(3 * 4096),
        },
};
