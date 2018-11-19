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
#include <lib/tipc/tipc.h>
#include <stdio.h>
#include <trusty_ipc.h>
#include <uapi/err.h>

#define TLOG_TAG "load-start"

int main(void) {
    int rc;
    uint8_t data;
    int data_len;
    handle_t port;
    handle_t chan;
    uevent_t uevt;
    uuid_t peer_uuid;

    data_len = sizeof(data);

    rc = port_create(LOADABLE_PORT, 1, 1, IPC_PORT_ALLOW_TA_CONNECT);
    if (rc < 0) {
        TLOGI("Failed (%d) to create port: %s\n", rc, LOADABLE_PORT);
        return rc;
    }

    port = (handle_t)rc;

    rc = wait(port, &uevt, -1);
    if (rc != NO_ERROR || !(uevt.event & IPC_HANDLE_POLL_READY)) {
        TLOGI("Port wait failed: %d(%d)\n", rc, uevt.event);
        goto err_port_wait;
    }

    rc = accept(uevt.handle, &peer_uuid);
    if (rc < 0) {
        TLOGI("Accept failed %d\n", rc);
        goto err_accept;
    }

    chan = (handle_t)rc;
    rc = wait(chan, &uevt, -1);
    if (rc < 0 || !(uevt.event & IPC_HANDLE_POLL_MSG)) {
        TLOGI("Channel wait failed: %d(%d)\n", rc, uevt.event);
        goto err_chan_wait;
    }

    rc = tipc_recv1(chan, data_len, &data, data_len);
    if (rc < data_len) {
        TLOGI("receive failed: %d\n", rc);
        goto err_recv;
    }

    rc = tipc_send1(chan, &data, data_len);
    if (rc < data_len) {
        TLOGI("send failed: %d\n", rc);
        goto err_send;
    }

    rc = 0;

err_send:
err_recv:
err_chan_wait:
    close(chan);
err_accept:
err_port_wait:
    close(port);
    return rc;
}
