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

#include <app_mgmt_test.h>
#include <trusty_app_manifest.h>

trusty_app_manifest_t TRUSTY_APP_MANIFEST_ATTRS trusty_app_manifest = {
        /* UUID : {c14672f9-6a19-4e3e-9b27-0cd6024de401} */
        .uuid = PORT_START_UUID,
        /* optional configuration options here */
        {
                TRUSTY_APP_CONFIG_MIN_HEAP_SIZE(1 * 4096),
                TRUSTY_APP_CONFIG_MGMT_FLAGS(
                        TRUSTY_APP_MGMT_FLAGS_DEFERRED_START),
        },
};

TRUSTY_APP_START_PORT(LOADABLE_START_PORT, IPC_PORT_ALLOW_TA_CONNECT);
