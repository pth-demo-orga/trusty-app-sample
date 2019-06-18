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
#include <lib/unittest/unittest.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <trusty_app_mgmt.h>
#include <trusty_unittest.h>
#include <uapi/err.h>
#include <uapi/trusty_uuid.h>

#define TLOG_TAG "app-mgmt-syscalls-test"
#define PORT_BASE "com.android.appmgmt-unittest.syscalls"

#define KERNEL_ADDRESS KERNEL_ASPACE_BASE
#define INVALID_USER_ADDRESS (0x1000)

#define TEST_DATA_BYTE 0x5a
#define PAGE_SIZE 4096

static uuid_t load_start_uuid = LOAD_START_UUID;
static uuid_t port_start_uuid = PORT_START_UUID;
static uuid_t builtin_uuid = BUILTIN_UUID;

#define PRINT_UUID(u)                                                \
    TLOGI("uuid:%02x-%02x-%02x-%02x%02x-%02x%02x%02x%02x%02x%02x\n", \
          (u)->time_low, (u)->time_mid, (u)->time_hi_and_version,    \
          (u)->clock_seq_and_node[0], (u)->clock_seq_and_node[1],    \
          (u)->clock_seq_and_node[2], (u)->clock_seq_and_node[3],    \
          (u)->clock_seq_and_node[4], (u)->clock_seq_and_node[5],    \
          (u)->clock_seq_and_node[6], (u)->clock_seq_and_node[7]);
/*
 * These tests make use of three loadable apps: load_start, port_start and
 * duplicate_port.
 *
 * load_start:
 *     - Starts when loaded
 *     - Waits on LOADABLE_PORT for a connection
 *     - Exits after echoing a message
 *
 * port_start:
 *     - Doesn't start when loaded
 *     - Starts on connection to LOADABLE_START_PORT
 *     - Waits on LOADABLE_START_PORT for a connection
 *     - Exits after echoing a message
 *
 * duplicate_port:
 *     - Declares LOADABLE_START_PORT in its manifest
 *     - Prints an error and exits after starting
 */

extern uint8_t _bin_load_start_begin[];
extern uint8_t _bin_load_start_end[];

extern uint8_t _bin_port_start_begin[];
extern uint8_t _bin_port_start_end[];

extern uint8_t _bin_duplicate_port_begin[];
extern uint8_t _bin_duplicate_port_end[];

/*
 * Best way we have now since app termination is asynchronous and there isn't
 * a good way to signal user space when an app has exited.
 */
int unregister_app_poll(uuid_t* app_uuid) {
    int rc;

    rc = unregister_app(app_uuid);

    while (rc == ERR_BUSY) {
        trusty_nanosleep(0, 0, 0);
        rc = unregister_app(app_uuid);
    }

    if (rc != NO_ERROR) {
        TLOGI("Failed(%d) to unregister app\n", rc);
        PRINT_UUID(app_uuid);
    }

    return rc;
}

TEST(AppMgmtSyscalls, RegisterAppZeroSize) {
    int rc;

    rc = register_app(_bin_port_start_begin, 0);
    EXPECT_EQ(ERR_INVALID_ARGS, rc);
}

TEST(AppMgmtSyscalls, RegisterAppInvalidUserAddress) {
    int rc;

    rc = register_app((void*)INVALID_USER_ADDRESS, PAGE_SIZE);
    EXPECT_EQ(ERR_FAULT, rc);
}

TEST(AppMgmtSyscalls, RegisterAppKernelAddress) {
    int rc;

    rc = register_app((void*)KERNEL_ADDRESS, PAGE_SIZE);
    EXPECT_EQ(ERR_FAULT, rc);
}

TEST(AppMgmtSyscalls, RegisterAppInvalidApp) {
    int rc;
    uint8_t invalid_app[PAGE_SIZE * 2];

    memset(invalid_app, 0xff, sizeof invalid_app);

    rc = register_app(invalid_app, sizeof invalid_app);
    EXPECT_EQ(ERR_NOT_VALID, rc);
}

/*
 * This test tests three things that are tightly related and difficult to test
 * separately:
 *
 * 1. App registration
 * 2. DEFERRED_START for loadable apps
 * 3. App unregistration
 */
TEST(AppMgmtSyscalls, RegisterUnregisterDeferredStartApp) {
    int rc;
    uint32_t img_size;

    /* Register port_start */
    img_size = _bin_port_start_end - _bin_port_start_begin;
    rc = register_app(_bin_port_start_begin, img_size);
    ASSERT_EQ(NO_ERROR, rc);

    /* Unregister port_start */
    rc = unregister_app_poll(&port_start_uuid);
    EXPECT_EQ(NO_ERROR, rc);

test_abort:
    return;
}

TEST(AppMgmtSyscalls, UnregisterAppNotRegistered) {
    int rc;
    uuid_t invalid_uuid;

    memset(&invalid_uuid, 0x4b, sizeof(uuid_t));

    rc = unregister_app(&invalid_uuid);
    EXPECT_EQ(ERR_NOT_FOUND, rc);
}

TEST(AppMgmtSyscalls, UnregisterAppInvalidBuiltin) {
    int rc;

    rc = unregister_app(&builtin_uuid);
    EXPECT_EQ(ERR_NOT_ALLOWED, rc);
}

TEST(AppMgmtSyscalls, UnregisterAppInvalidUserAddress) {
    int rc;

    rc = unregister_app((uuid_t*)INVALID_USER_ADDRESS);
    EXPECT_EQ(ERR_FAULT, rc);
}

TEST(AppMgmtSyscalls, UnregisterAppKernelAddress) {
    int rc;

    rc = unregister_app((uuid_t*)KERNEL_ADDRESS);
    EXPECT_EQ(ERR_FAULT, rc);
}

TEST(AppMgmtSyscalls, RegisterAppLoadStart) {
    int rc;
    handle_t chan = INVALID_IPC_HANDLE;
    uevent_t uevt;
    uint32_t img_size;
    uint8_t data = TEST_DATA_BYTE;
    uint8_t recv_data;

    /* Register load_start */
    img_size = _bin_load_start_end - _bin_load_start_begin;
    rc = register_app(_bin_load_start_begin, img_size);
    ASSERT_EQ(NO_ERROR, rc);

    /* load_start should start after it is registered */
    rc = connect(LOADABLE_PORT, IPC_CONNECT_WAIT_FOR_PORT);
    ASSERT_GE(rc, 0);

    /* Send a msg and receive the echo response */
    chan = (handle_t)rc;
    rc = tipc_send1(chan, &data, sizeof(data));
    ASSERT_EQ(sizeof(data), rc);

    rc = wait(chan, &uevt, INFINITE_TIME);
    ASSERT_EQ(NO_ERROR, rc);
    ASSERT_NE(0, IPC_HANDLE_POLL_MSG & uevt.event);

    rc = tipc_recv1(chan, sizeof(data), &recv_data, sizeof(recv_data));
    ASSERT_EQ(sizeof(data), rc);
    ASSERT_EQ(data, recv_data);

    /* load_start should exit after echoing the msg */

test_abort:
    close(chan);
    rc = unregister_app_poll(&load_start_uuid);
    EXPECT_EQ(NO_ERROR, rc);
}

TEST(AppMgmtSyscalls, RegisterAppPortStart) {
    int rc;
    uevent_t uevt;
    handle_t chan = INVALID_IPC_HANDLE;
    uint32_t img_size;
    uint8_t data = TEST_DATA_BYTE;
    uint8_t recv_data;

    /* Register port_start */
    img_size = _bin_port_start_end - _bin_port_start_begin;
    rc = register_app(_bin_port_start_begin, img_size);
    ASSERT_EQ(NO_ERROR, rc);

    /* Start and connect to port_start */
    rc = connect(LOADABLE_START_PORT, 0);
    ASSERT_GE(rc, 0);
    chan = (handle_t)rc;

    /* Send a msg and receive the echo response */
    rc = tipc_send1(chan, &data, sizeof(data));
    ASSERT_EQ(sizeof(data), rc);

    rc = wait(chan, &uevt, INFINITE_TIME);
    ASSERT_EQ(NO_ERROR, rc);
    ASSERT_NE(0, IPC_HANDLE_POLL_MSG & uevt.event);

    rc = tipc_recv1(chan, sizeof(data), &recv_data, sizeof(recv_data));
    ASSERT_EQ(sizeof(data), rc);
    ASSERT_EQ(data, recv_data);

    /* port_start should exit after echoing the msg */

test_abort:
    close(chan);
    rc = unregister_app_poll(&port_start_uuid);
    EXPECT_EQ(NO_ERROR, rc);
    return;
}

TEST(AppMgmtSyscalls, RegisterAppPortStartWaitingConnection) {
    int rc;
    uevent_t uevt;
    handle_t chan = INVALID_IPC_HANDLE;
    uint32_t img_size;
    uint8_t data = TEST_DATA_BYTE;
    uint8_t recv_data;

    /*
     * Create a connection that is waiting for a port registered by an app
     * that will be loaded (i.e. port_start).
     */
    rc = connect(LOADABLE_START_PORT,
                 IPC_CONNECT_ASYNC | IPC_CONNECT_WAIT_FOR_PORT);
    ASSERT_GE(rc, 0);
    chan = (handle_t)rc;

    /* Register port_start */
    img_size = _bin_port_start_end - _bin_port_start_begin;
    rc = register_app(_bin_port_start_begin, img_size);
    ASSERT_EQ(NO_ERROR, rc);

    /*
     * The app should be started by the waiting connectiong. Wait for the
     * connection to stablish
     */
    rc = wait(chan, &uevt, INFINITE_TIME);
    ASSERT_EQ(NO_ERROR, rc);
    ASSERT_NE(0, IPC_HANDLE_POLL_READY & uevt.event);

    /* Send a msg and receive the echo response */
    rc = tipc_send1(chan, &data, sizeof(data));
    ASSERT_EQ(sizeof(data), rc);

    rc = wait(chan, &uevt, INFINITE_TIME);
    ASSERT_EQ(NO_ERROR, rc);
    ASSERT_NE(0, IPC_HANDLE_POLL_MSG & uevt.event);

    rc = tipc_recv1(chan, sizeof(data), &recv_data, sizeof(recv_data));
    ASSERT_EQ(sizeof(data), rc);
    ASSERT_EQ(data, recv_data);

    /* port_start should exit after echoing the msg */

test_abort:
    close(chan);
    rc = unregister_app_poll(&port_start_uuid);
    EXPECT_EQ(NO_ERROR, rc);
    return;
}

TEST(AppMgmtSyscalls, RegisterAppAlreadyExists) {
    int rc;
    uint32_t img_size;

    /* Register port_start */
    img_size = _bin_port_start_end - _bin_port_start_begin;
    rc = register_app(_bin_port_start_begin, img_size);
    ASSERT_EQ(NO_ERROR, rc);

    /* Try to register port_start again */
    rc = register_app(_bin_port_start_begin, img_size);
    ASSERT_EQ(ERR_ALREADY_EXISTS, rc);

test_abort:
    rc = unregister_app_poll(&port_start_uuid);
    EXPECT_EQ(NO_ERROR, rc);
    return;
}

TEST(AppMgmtSyscalls, RegisterAppPortAlreadyExists) {
    int rc;
    uint32_t img_size;

    /* Register port_start */
    img_size = _bin_port_start_end - _bin_port_start_begin;
    rc = register_app(_bin_port_start_begin, img_size);
    ASSERT_EQ(NO_ERROR, rc);

    /*
     * Try to register duplicate port. This should fail as duplicate_port tries
     * to register a port (LOADABLE_START_PORT) that is already registered by
     * port_start.
     */
    img_size = _bin_duplicate_port_end - _bin_duplicate_port_begin;
    rc = register_app(_bin_duplicate_port_begin, img_size);
    ASSERT_EQ(ERR_ALREADY_EXISTS, rc);

test_abort:
    rc = unregister_app_poll(&port_start_uuid);
    EXPECT_EQ(NO_ERROR, rc);
    return;
}

TEST(AppMgmtSyscalls, UnregisterAppRunning) {
    int rc;
    uevent_t uevt;
    handle_t chan = INVALID_IPC_HANDLE;
    uint32_t img_size;
    uint8_t data = TEST_DATA_BYTE;
    uint8_t recv_data;

    /* Register port_start */
    img_size = _bin_port_start_end - _bin_port_start_begin;
    rc = register_app(_bin_port_start_begin, img_size);
    ASSERT_EQ(NO_ERROR, rc);

    /* Start the app by connecting to it */
    rc = connect(LOADABLE_START_PORT, IPC_CONNECT_WAIT_FOR_PORT);
    ASSERT_GE(rc, 0);
    chan = (handle_t)rc;

    /* Try to unregister the app while it is sitll running */
    rc = unregister_app(&port_start_uuid);
    ASSERT_EQ(ERR_BUSY, rc);

    /* Send a msg and receive the echo response */
    rc = tipc_send1(chan, &data, sizeof(data));
    ASSERT_EQ(sizeof(data), rc);

    rc = wait(chan, &uevt, INFINITE_TIME);
    ASSERT_EQ(NO_ERROR, rc);
    ASSERT_NE(0, IPC_HANDLE_POLL_MSG & uevt.event);

    rc = tipc_recv1(chan, sizeof(data), &recv_data, sizeof(recv_data));
    ASSERT_EQ(sizeof(data), rc);
    ASSERT_EQ(data, recv_data);

    /* port_start should exit after echoing the msg */

test_abort:
    /* Unregister port_start */
    rc = unregister_app_poll(&port_start_uuid);
    EXPECT_EQ(NO_ERROR, rc);
    return;
}

PORT_TEST(AppMgmtSyscalls, PORT_BASE)
