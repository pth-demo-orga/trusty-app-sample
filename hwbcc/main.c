/*
 * Copyright (C) 2021 The Android Open Source Project
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

#define TLOG_TAG "hwbcc-srv-impl"

#include <lib/hwbcc/common/swbcc.h>
#include <lib/hwbcc/srv/srv.h>
#include <lib/tipc/tipc_srv.h>
#include <lk/err_ptr.h>
#include <trusty_log.h>
#include <uapi/err.h>

int main(void) {
    int rc;
    struct tipc_hset* hset;
    struct hwbcc_ops ops;

    hset = tipc_hset_create();
    if (IS_ERR(hset)) {
        TLOGE("failed (%d) to create handle set\n", PTR_ERR(hset));
        return PTR_ERR(hset);
    }

    ops.init = swbcc_init;
    ops.close = swbcc_close;
    ops.sign_mac = swbcc_sign_mac;
    ops.get_bcc = swbcc_get_bcc;
    rc = add_hwbcc_service(hset, &ops);
    if (rc != NO_ERROR) {
        TLOGE("failed (%d) to initialize system state service\n", rc);
        return rc;
    }

    return tipc_run_event_loop(hset);
}
