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

#include <lib/unittest/unittest.h>
#include <trusty_app_mgmt.h>
#include <trusty_unittest.h>
#include <uapi/err.h>

#define TLOG_TAG "app-mgmt-syscalls-test-aux"
#define PORT_BASE "com.android.appmgmt-unittest.syscalls.aux"

TEST(AppMgmtSyscalls, RegisterAppNoAccess) {
    int rc;

    rc = register_app((void*)USER_ASPACE_BASE, 0x8000);
    EXPECT_EQ(ERR_ACCESS_DENIED, rc);
}

TEST(AppMgmtSyscalls, UnregisterAppNoAccess) {
    int rc;
    uuid_t uuid;

    memset(&uuid, 0xff, sizeof(uuid_t));
    rc = unregister_app(&uuid);
    EXPECT_EQ(ERR_ACCESS_DENIED, rc);
}

PORT_TEST(AppMgmtSyscalls, PORT_BASE);
