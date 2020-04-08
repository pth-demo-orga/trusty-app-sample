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

#include <app_mgmt_port_consts.h>
#include <app_mgmt_test.h>
#include <assert.h>
#include <lib/tipc/tipc.h>
#include <lib/unittest/unittest.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <trusty_ipc.h>
#include <trusty_unittest.h>
#include <uapi/err.h>

#define TLOG_TAG "app-mgmt-test-client"
#define PORT_BASE "com.android.appmgmt-unittest.appmngr"

/*
 * These are expected to occur/elapse in passing test cases and as such there is
 * a trade off between the degree of confidence provided by the tests and the
 * runtime of the tests.
 */
#define EXPECTED_TIMEOUT_MS 500
#define WAIT_FOR_APP_SLEEP_NS 500000000

/*
 * These tests makes use of one client TA (this file) and 4 test server TAs:
 * boot-start-srv, never-start-srv, restart-srv and port-tart-srv.
 *
 * boot-start-srv:
 *  - Starts at boot
 *  - Creates BOOT_START_PORT
 *  - Never exits
 *  - Does not restart at exit
 *
 * never-start-srv:
 *  - Should never be started
 *  - Creates NEVER_START_PORT (if ever started i.e. failure case)
 *  - Exits after receiving a connection
 *  - Restarts after exiting
 *
 * restart-srv:
 *  - Starts at boot
 *  - Creates RESTART_PORT
 *  - Exits after receiving a connection
 *  - Restarts after exiting
 *
 * start-port-srv:
 *  - Doesn't start at boot
 *  - Starts on connections to START_PORT
 *  - Creates START_PORT, CTRL_PORT and SHUTDOWN_PORT
 *  - Exits after receiving a CMD_EXIT command on START_PORT or a connection on
 *    SHUTDOWN_PORT
 *  - Does not restart at exit
 *
 */

static bool port_start_srv_running(void) {
    int rc;

    trusty_nanosleep(0, 0, WAIT_FOR_APP_SLEEP_NS);
    rc = connect(CTRL_PORT, IPC_CONNECT_ASYNC);
    close((handle_t)rc);
    return rc >= 0;
}

static void chan_send_cmd(handle_t chan, uint8_t cmd) {
    uint8_t rsp;
    uevent_t uevt;

    if (HasFailure())
        return;

    ASSERT_EQ(sizeof(cmd), tipc_send1(chan, &cmd, sizeof(cmd)));
    ASSERT_EQ(NO_ERROR, wait(chan, &uevt, INFINITE_TIME));
    ASSERT_NE(0, uevt.event & IPC_HANDLE_POLL_MSG);
    ASSERT_EQ(sizeof(rsp), tipc_recv1(chan, sizeof(rsp), &rsp, sizeof(rsp)));
    ASSERT_EQ(RSP_OK, rsp);

test_abort:
    return;
}

typedef enum {
    /* Accepted on connect and served by port-start-srv */
    MAIN_CHAN,

    /* Not accepted on connect. Put on pending list when established */
    PEND_CHAN,

    /* Not accepted on connect. Put on waiting list when established */
    WAIT_CHAN,

    CHAN_COUNT,
} chan_idx_t;

typedef struct {
    handle_t chans[CHAN_COUNT];
} AppMgrPortStart_t;

static void send_cmd(AppMgrPortStart_t* state, chan_idx_t idx, uint8_t cmd) {
    assert(idx < CHAN_COUNT);

    if (HasFailure())
        return;

    chan_send_cmd(state->chans[idx], cmd);

test_abort:
    return;
}

static void establish_unhandled_channel(AppMgrPortStart_t* state,
                                        chan_idx_t idx) {
    int rc;
    uevent_t uevt;
    handle_t chan = INVALID_IPC_HANDLE;

    assert(idx < CHAN_COUNT);

    if (HasFailure())
        return;

    rc = connect(START_PORT, IPC_CONNECT_ASYNC);
    ASSERT_GE(rc, 0);
    chan = (handle_t)rc;

    /* Make sure port-start-srv does not accept the connection */
    ASSERT_EQ(ERR_TIMED_OUT, wait(chan, &uevt, EXPECTED_TIMEOUT_MS));

    state->chans[idx] = chan;

test_abort:
    return;
}

static void close_channel(AppMgrPortStart_t* state, chan_idx_t idx) {
    assert(idx < CHAN_COUNT);

    if (HasFailure())
        return;

    close(state->chans[idx]);
    state->chans[idx] = INVALID_IPC_HANDLE;
}

static void send_exit(AppMgrPortStart_t* state, chan_idx_t idx) {
    assert(idx < CHAN_COUNT);

    if (HasFailure())
        return;

    send_cmd(state, idx, CMD_EXIT);
    close_channel(state, idx);
}

static void wait_and_exit(AppMgrPortStart_t* state, chan_idx_t idx) {
    uevent_t uevt;

    assert(idx < CHAN_COUNT);

    if (HasFailure())
        return;

    ASSERT_EQ(NO_ERROR, wait(state->chans[idx], &uevt, INFINITE_TIME));
    ASSERT_NE(0, IPC_HANDLE_POLL_READY & uevt.event);

    send_exit(state, idx);

test_abort:
    return;
}

static void AppMgrPortStart_SetUp(AppMgrPortStart_t* state) {
    int rc;
    uevent_t uevt;
    handle_t chan;

    for (size_t i = 0; i < CHAN_COUNT; i++) {
        state->chans[i] = INVALID_IPC_HANDLE;
    }

    /* Shutdown port-start-srv in case it is running from a previous test */
    rc = connect(SHUTDOWN_PORT, IPC_CONNECT_ASYNC);
    if (rc > 0) {
        /* SHUTDOWN_PORT exists so the srv was running. Wait for it to exit */
        chan = (handle_t)rc;
        rc = wait(chan, &uevt, INFINITE_TIME);
        close(chan);
        ASSERT_GE(rc, 0);
        ASSERT_NE(0, uevt.event & IPC_HANDLE_POLL_HUP);
    }

    /* port-start-srv should not be running */
    ASSERT_EQ(false, port_start_srv_running());

    /* Start and connect to port-start-srv */
    rc = connect(START_PORT, 0);
    ASSERT_GE(rc, 0);

    state->chans[MAIN_CHAN] = (handle_t)rc;

test_abort:
    return;
}

static void AppMgrPortStart_TearDown(AppMgrPortStart_t* state) {
    ASSERT_EQ(false, HasFailure());

    /* port-start-srv should not be running at the end of a test */
    ASSERT_EQ(false, port_start_srv_running());

    for (size_t i = 0; i < CHAN_COUNT; i++) {
        ASSERT_EQ(INVALID_IPC_HANDLE, state->chans[i]);
    }

test_abort:
    for (size_t i = 0; i < CHAN_COUNT; i++) {
        close(state->chans[i]);
    }
}

/* Apps with deferred start should not start at boot */
TEST(AppMgrBoot, BootStartNegative) {
    int rc;

    /* never-start-srv should not be running */
    rc = connect(NEVER_START_PORT, IPC_CONNECT_ASYNC);
    EXPECT_LT(rc, 0);
    close((handle_t)rc);
}

/* Apps without deferred should start at boot */
TEST(AppMgrBoot, BootStartPositive) {
    int rc;

    /* boot-start-srv should be running from boot */
    rc = connect(BOOT_START_PORT, IPC_CONNECT_ASYNC);
    EXPECT_GE(rc, 0);
    close((handle_t)rc);
}

/* Apps with automatic restart should restart after exiting */
TEST(AppMgrRestart, AppRestartPositive) {
    int rc;
    uevent_t uevt;
    handle_t chan = INVALID_IPC_HANDLE;

    /* restart-srv should be running from boot or a previous restart */
    rc = connect(RESTART_PORT, IPC_CONNECT_ASYNC | IPC_CONNECT_WAIT_FOR_PORT);
    ASSERT_GE(rc, 0);

    /* Wait for restart-srv to initiate shutdown */
    chan = (handle_t)rc;
    ASSERT_EQ(NO_ERROR, wait(chan, &uevt, INFINITE_TIME));
    ASSERT_NE(0, IPC_HANDLE_POLL_HUP & uevt.event);
    close(chan);

    /* restart-srv should eventually restart */
    rc = connect(RESTART_PORT, IPC_CONNECT_ASYNC | IPC_CONNECT_WAIT_FOR_PORT);
    ASSERT_GE(rc, 0);
    chan = (handle_t)rc;

test_abort:
    close(chan);
}

/*
 * Apps without automatic restart should not restart after exiting
 * Start ports should start an app on connection
 */
TEST(AppMgrRestart, AppRestartNegativePortStartPositive) {
    int rc;
    handle_t chan = INVALID_IPC_HANDLE;

    /* Start and connect to port-start-srv */
    rc = connect(START_PORT, 0);
    ASSERT_GE(rc, 0);
    chan = (handle_t)rc;

    /* Shutdown port-start-srv */
    chan_send_cmd(chan, CMD_EXIT);
    ASSERT_EQ(false, HasFailure());

    /* port-start-srv should not restart */
    ASSERT_EQ(false, port_start_srv_running());

test_abort:
    close((handle_t)rc);
}

/* Regular ports should not start an app on connection */
TEST(AppMgrPortStartNegative, PortStartNegative) {
    int rc;

    /* A connection to CTRL_PORT should not start port-start-srv */
    rc = connect(CTRL_PORT, IPC_CONNECT_ASYNC);
    EXPECT_LT(rc, 0);
    close((handle_t)rc);
}

/* Start ports with closed pending connections should not start an app */
TEST_F(AppMgrPortStart, PortStartPendingNegative) {
    /* Create a pending connection */
    establish_unhandled_channel(_state, PEND_CHAN);

    /* Close the pending connection */
    close_channel(_state, PEND_CHAN);

    /* Close the main channel and shutdown port-start-srv */
    send_exit(_state, MAIN_CHAN);
}

/* Start ports with pending connections should start an app */
TEST_F(AppMgrPortStart, PortStartPendingPositive) {
    /* Create a pending connection */
    establish_unhandled_channel(_state, PEND_CHAN);

    /* Close the main channel and shutdown port-start-srv */
    send_exit(_state, MAIN_CHAN);

    /*
     * Wait for port-start-srv to restart due to the pending connection and then
     * shut it down
     */
    wait_and_exit(_state, PEND_CHAN);
}

/* Closed connections waiting for a start port should not start an app */
TEST_F(AppMgrPortStart, PortStartWaitingNegative) {
    /* Make port-start-srv close START_PORT */
    send_cmd(_state, MAIN_CHAN, CMD_CLOSE_PORT);

    /* Create a waiting connection */
    establish_unhandled_channel(_state, WAIT_CHAN);

    /* Close the waiting connection */
    close_channel(_state, WAIT_CHAN);

    /* Close the main channel and shutdown port-start-srv */
    send_exit(_state, MAIN_CHAN);
}

/* Connections waiting for a start port should start an app */
TEST_F(AppMgrPortStart, PortStartWaitingPositive) {
    /* Make port-start-srv close START_PORT */
    send_cmd(_state, MAIN_CHAN, CMD_CLOSE_PORT);

    /* Create a waiting connection */
    establish_unhandled_channel(_state, WAIT_CHAN);

    /* Close the main channel and shutdown port-start-srv */
    send_exit(_state, MAIN_CHAN);

    /*
     * Wait for port-start-srv to restart due to the waiting connection and then
     * shut it down
     */
    wait_and_exit(_state, WAIT_CHAN);
}

/*
 * Closed waiting connections that were pending on a start port should not start
 * an app
 */
TEST_F(AppMgrPortStart, PortStartPendingToWaitingNegative) {
    /* Create a pending connection */
    establish_unhandled_channel(_state, PEND_CHAN);

    /*
     * Make port-start-srv close START_PORT (the pending connection becomes
     * waiting)
     */
    send_cmd(_state, MAIN_CHAN, CMD_CLOSE_PORT);

    /* Close the waiting connection */
    close_channel(_state, PEND_CHAN);

    /* Close the main channel and shutdown port-start-srv */
    send_exit(_state, MAIN_CHAN);
}

/*
 * Waiting connections that were pending on a start port should start an app
 */
TEST_F(AppMgrPortStart, PortStartPendingToWaitingPositive) {
    /* Create a pending connection */
    establish_unhandled_channel(_state, PEND_CHAN);

    /*
     * Make port-start-srv close START_PORT (the pending connection becomes
     * waiting)
     */
    send_cmd(_state, MAIN_CHAN, CMD_CLOSE_PORT);

    /* Close the main channel and shutdown port-start-srv */
    send_exit(_state, MAIN_CHAN);

    /*
     * Wait for port-start-srv to restart due to the waiting connection and then
     * shut it down
     */
    wait_and_exit(_state, PEND_CHAN);
}

/*
 * Start ports with closed pending connections that were waiting for the port
 * should not start an app
 */
TEST_F(AppMgrPortStart, PortStartWaitingToPendingNegative) {
    /* Make port-start-srv close START_PORT */
    send_cmd(_state, MAIN_CHAN, CMD_CLOSE_PORT);

    /* Create a waiting connection */
    establish_unhandled_channel(_state, WAIT_CHAN);

    /*
     * Make port-start-srv open START_PORT (the waiting connection becomes
     * pending)
     */
    send_cmd(_state, MAIN_CHAN, CMD_OPEN_PORT);

    /* Close the pending connection */
    close_channel(_state, WAIT_CHAN);

    /* Close the main channel and shutdown port-start-srv */
    send_exit(_state, MAIN_CHAN);
}

/*
 * Start ports with pending connections that were waiting for the port should
 * start an app
 */
TEST_F(AppMgrPortStart, PortStartWaitingToPendingPositive) {
    /* Make port-start-srv close START_PORT */
    send_cmd(_state, MAIN_CHAN, CMD_CLOSE_PORT);

    /* Create a waiting connection */
    establish_unhandled_channel(_state, WAIT_CHAN);

    /*
     * Make port-start-srv open START_PORT (the waiting connection becomes
     * pending)
     */
    send_cmd(_state, MAIN_CHAN, CMD_OPEN_PORT);

    /* Close the main channel and shutdown port-start-srv */
    send_exit(_state, MAIN_CHAN);

    /*
     * Wait for port-start-srv to restart due to the pending connection and then
     * shut it down
     */
    wait_and_exit(_state, WAIT_CHAN);
}

/*
 * Closed connections waiting for a start port with closed pending connections
 * should not start an app
 */
TEST_F(AppMgrPortStart, PortStartPendingWaitingNegative) {
    /* Create a pending connection */
    establish_unhandled_channel(_state, PEND_CHAN);

    /*
     * Make port-start-srv close START_PORT (the pending connection becomes
     * waiting)
     */
    send_cmd(_state, MAIN_CHAN, CMD_CLOSE_PORT);

    /* Create a waiting connection */
    establish_unhandled_channel(_state, WAIT_CHAN);

    /* Close the first waiting connection */
    close_channel(_state, PEND_CHAN);

    /* Close the second waiting connection */
    close_channel(_state, WAIT_CHAN);

    /* Close the main channel and shutdown port-start-srv */
    send_exit(_state, MAIN_CHAN);
}

/*
 * Connections waiting for a start port with pending connections should start
 * an app
 */
TEST_F(AppMgrPortStart, PortStartPendingWaitingPositive) {
    /* Create a pending connection */
    establish_unhandled_channel(_state, PEND_CHAN);

    /*
     * Make port-start-srv close START_PORT (the pending connection becomes
     * waiting)
     */
    send_cmd(_state, MAIN_CHAN, CMD_CLOSE_PORT);

    /* Create a waiting connection */
    establish_unhandled_channel(_state, WAIT_CHAN);

    /* Close the main channel and shutdown port-start-srv */
    send_exit(_state, MAIN_CHAN);

    /*
     * Wait for port-start-srv to restart due to the first waiting connection
     * and then shut it down
     */
    wait_and_exit(_state, PEND_CHAN);

    /*
     * wait for port-start-srv to restart due to the second waiting connection
     * and then shut it down
     */
    wait_and_exit(_state, WAIT_CHAN);
}

/*
 * Connections waiting for a start port with closed pending connections should
 * start an app
 */
TEST_F(AppMgrPortStart, PortStartPendingClosedWaitingPositive) {
    /* Create a pending connection */
    establish_unhandled_channel(_state, PEND_CHAN);

    /*
     * Make port-start-srv close START_PORT (the pending connection becomes
     * waiting)
     */
    send_cmd(_state, MAIN_CHAN, CMD_CLOSE_PORT);

    /* Create a waiting connection */
    establish_unhandled_channel(_state, WAIT_CHAN);

    /* Close the first waiting connection */
    close_channel(_state, PEND_CHAN);

    /* Close the main channel and shutdown port-start-srv */
    send_exit(_state, MAIN_CHAN);

    /*
     * wait for port-start-srv to restart due to the second waiting connection
     * and then shut it down
     */
    wait_and_exit(_state, WAIT_CHAN);
}

/*
 * Start ports with pending connections and with closed connections waiting for
 * the port should start an app
 */
TEST_F(AppMgrPortStart, PortStartPendingWaitingClosedPositive) {
    /* Create a pending connection */
    establish_unhandled_channel(_state, PEND_CHAN);

    /*
     * Make port-start-srv close START_PORT (the pending connection becomes
     * waiting)
     */
    send_cmd(_state, MAIN_CHAN, CMD_CLOSE_PORT);

    /* Create a waiting connection */
    establish_unhandled_channel(_state, WAIT_CHAN);

    /* Close the second waiting connection */
    close_channel(_state, WAIT_CHAN);

    /* Close the main channel and shutdown port-start-srv */
    send_exit(_state, MAIN_CHAN);

    /*
     * wait for port-start-srv to restart due to the first waiting connection
     * and then shut it down
     */
    wait_and_exit(_state, PEND_CHAN);
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
