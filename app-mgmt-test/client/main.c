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
#include <lib/unittest/unittest.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <trusty_ipc.h>
#include <trusty_unittest.h>

#define TLOG_TAG "app-mgmt-test-client"
#define PORT_BASE "com.android.appmgmt-unittest.appmngr"

TEST(AppMgrBoot, BootStartNegative) {
    int rc;
    /* Check never-start-srv is not running */
    rc = connect(NEVER_START_PORT, IPC_CONNECT_ASYNC);
    EXPECT_LT(rc, 0);
    close(rc);
}

TEST(AppMgrBoot, BootStartPositive) {
    int rc;
    /* Check boot-start-srv is running */
    rc = connect(BOOT_START_PORT, IPC_CONNECT_ASYNC);
    EXPECT_GE(rc, 0);
    close(rc);
}

static bool run_appmngr_tests(struct unittest* test) {
    return RUN_ALL_TESTS();
}

static bool run_appmngr_stress_tests(struct unittest* test) {
    while (RUN_ALL_TESTS()) {
    }

    return false;
}

int main(void) {
    static struct unittest appmgmt_unittests[] = {
            {
                    .port_name = PORT_BASE,
                    .run_test = run_appmngr_tests,
            },
            {
                    .port_name = PORT_BASE ".stress",
                    .run_test = run_appmngr_stress_tests,
            },
    };
    struct unittest* unittests[countof(appmgmt_unittests)];

    for (size_t i = 0; i < countof(appmgmt_unittests); i++)
        unittests[i] = &appmgmt_unittests[i];

    return unittest_main(unittests, countof(unittests));
}
