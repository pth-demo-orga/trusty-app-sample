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

#include <stddef.h>
#include <stdio.h>
#include <trusty_app_manifest.h>

trusty_app_manifest_t TRUSTY_APP_MANIFEST_ATTRS trusty_app_manifest = {
        /* UUID : {ef45baea-e0b0-477c-9494-d59edd047f46} */
        {0xef55baea,
         0xe0b0,
         0x477c,
         {0x94, 0x94, 0xd5, 0x9e, 0xdd, 0x04, 0x7f, 0x46}},

        {TRUSTY_APP_CONFIG_MIN_STACK_SIZE(4096),

         TRUSTY_APP_CONFIG_MIN_HEAP_SIZE(4096),

         TRUSTY_APP_CONFIG_MAP_MEM(1, 0x70000000, 0x1000),
         TRUSTY_APP_CONFIG_MAP_MEM(2, 0x70010000, 0x100),
         TRUSTY_APP_CONFIG_MAP_MEM(3, 0x70020000, 0x4)}};
