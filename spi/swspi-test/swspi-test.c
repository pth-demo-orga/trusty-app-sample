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

#include <assert.h>
#include <interface/spi/spi_loopback.h>
#include <interface/spi/spi_test.h>
#include <lib/spi/client/spi.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/auxv.h>
#include <time.h>
#include <uapi/err.h>

#define TLOG_TAG "swspi-test"
#include <trusty_unittest.h>

#define MAX_NUM_CMDS 32
#define MAX_TOTAL_PAYLOAD 1024
#define TXRX_SIZE 1024
#define CLK_SPEED 1000000 /* 1 MHz */
#define PAGE_SIZE getauxval(AT_PAGESZ)

enum {
    SPI_TEST_DEV_IDX = 0,
    SPI_LOOPBACK_DEV_IDX = 1,
    SPI_DEV_COUNT,
};

/*
 * TODO: There is currently no way to close a SPI devices, so allocate global
 * instances and open them once.
 */
struct spi_test_dev {
    struct spi_dev dev;
    const char* name;
    bool initialized;
    bool loopback;
};

static struct spi_test_dev devs[SPI_DEV_COUNT] = {
        /*
         * This device calculates an 8-bit digest of TX buffer, seeds rand()
         * with that digest, fills RX with random bytes, and sends it back to
         * us. If we initiate a receive-only transfer, this device uses seed 0.
         */
        [SPI_TEST_DEV_IDX] =
                {
                        .name = SPI_TEST_PORT,
                },
        [SPI_LOOPBACK_DEV_IDX] =
                {
                        .name = SPI_LOOPBACK_PORT,
                        .loopback = true,
                },
};

static int spi_dev_init_once(int idx) {
    int rc;
    struct spi_test_dev* test_dev;

    test_dev = &devs[idx];
    if (test_dev->initialized) {
        return 0;
    }

    rc = spi_dev_open(&test_dev->dev, test_dev->name, MAX_NUM_CMDS,
                      MAX_TOTAL_PAYLOAD);

    if (rc == NO_ERROR) {
        test_dev->initialized = true;
    }

    return rc;
}

/* calculate an 8-bit digest of a buffer */
static uint8_t digest(uint8_t* buf, size_t sz) {
    uint8_t digest = 0;

    for (size_t i = 0; i < sz; i++) {
        /* rotate right one bit */
        digest = digest >> 1 | (digest & 0x1) << 7;
        digest ^= buf[i];
    }
    return digest;
}

/* fill buffer with random bytes generated using a given seed */
static void rand_buf(uint8_t* buf, size_t sz, uint8_t seed) {
    /* seed RNG */
    srand(seed);

    for (size_t i = 0; i < sz; i++) {
        buf[i] = rand() % 0xff;
    }
}

static int exec_xfer(struct spi_test_dev* test_dev, size_t len) {
    /* contains expected received buffer from a data transfer */
    static uint8_t result[TXRX_SIZE];

    int rc;
    void* tx = NULL;
    void* rx = NULL;
    size_t failed;
    uint8_t tx_seed;
    uint8_t rx_seed;
    struct spi_dev* dev = &test_dev->dev;
    bool loopback = test_dev->loopback;

    rc = spi_add_set_clk_cmd(dev, CLK_SPEED, NULL);
    EXPECT_EQ(rc, 0);

    rc = spi_add_cs_assert_cmd(dev);
    EXPECT_EQ(rc, 0);

    rc = spi_add_data_xfer_cmd(dev, &tx, &rx, len);
    EXPECT_EQ(rc, 0);

    rc = spi_add_cs_deassert_cmd(dev);
    EXPECT_EQ(rc, 0);

    /* fill out TX and expected RX */
    tx_seed = len % 0xff; /* to vary generated byte sequences a little */
    rand_buf(tx, len, tx_seed);
    if (loopback) {
        memcpy(result, tx, len);
    } else {
        rx_seed = digest(tx, len);
        rand_buf(result, len, rx_seed);
    }

    rc = spi_exec_cmds(dev, &failed);
    EXPECT_EQ(rc, 0);

    return memcmp(result, rx, len);
}

typedef struct {
    struct spi_test_dev* test_dev;
} swspi_t;

TEST_F_SETUP(swspi) {
    int rc;
    int idx = *((const int*)GetParam());
    assert(0 <= idx && idx < SPI_DEV_COUNT);

    rc = spi_dev_init_once(idx);
    ASSERT_EQ(rc, 0);

    _state->test_dev = &devs[idx];
    spi_clear_cmds(&_state->test_dev->dev);

test_abort:;
}

TEST_F_TEARDOWN(swspi) {}

TEST_P(swspi, add_cmd) {
    int rc;
    void* tx = NULL;
    void* rx = NULL;
    uint64_t* clk_hz = NULL;
    struct spi_dev* dev = &_state->test_dev->dev;

    rc = spi_add_cs_assert_cmd(dev);
    EXPECT_EQ(rc, 0);

    rc = spi_add_data_xfer_cmd(dev, &tx, &rx, 1);
    EXPECT_EQ(rc, 0);
    EXPECT_NE(tx, NULL);
    EXPECT_NE(rx, NULL);
    tx = NULL;
    rx = NULL;

    rc = spi_add_data_xfer_cmd(dev, &tx, NULL, 1);
    EXPECT_EQ(rc, 0);
    EXPECT_NE(tx, NULL);
    tx = NULL;

    rc = spi_add_data_xfer_cmd(dev, NULL, &rx, 1);
    EXPECT_EQ(rc, 0);
    EXPECT_NE(rx, NULL);
    rx = NULL;

    rc = spi_add_data_xfer_cmd(dev, &tx, &tx, 1);
    EXPECT_EQ(rc, 0);
    EXPECT_NE(tx, NULL);
    tx = NULL;

    rc = spi_add_data_xfer_cmd(dev, NULL, NULL, 0);
    EXPECT_EQ(rc, 0);

    rc = spi_add_cs_deassert_cmd(dev);
    EXPECT_EQ(rc, 0);

    rc = spi_add_set_clk_cmd(dev, CLK_SPEED, NULL);
    EXPECT_EQ(rc, 0);

    rc = spi_add_set_clk_cmd(dev, CLK_SPEED, &clk_hz);
    EXPECT_EQ(rc, 0);
}

TEST_P(swspi, add_cmd_null) {
    /* NULL device */
    int rc = spi_add_cs_assert_cmd(NULL);
    EXPECT_EQ(rc, ERR_BAD_HANDLE);
}

TEST_P(swspi, add_cmd_out_of_bounds) {
    int rc;
    struct spi_dev* dev = &_state->test_dev->dev;

    for (size_t i = 0; i < MAX_NUM_CMDS; i += 2) {
        rc = spi_add_cs_assert_cmd(dev);
        EXPECT_EQ(rc, 0);
        rc = spi_add_cs_deassert_cmd(dev);
        EXPECT_EQ(rc, 0);
    }

    rc = spi_add_cs_assert_cmd(dev);
    EXPECT_EQ(rc, ERR_OUT_OF_RANGE);
}

TEST_P(swspi, add_xfer_too_big) {
    int rc;
    void* tx;
    void* rx;
    size_t failed;
    struct spi_dev* dev = &_state->test_dev->dev;

    rc = spi_add_cs_assert_cmd(dev);
    EXPECT_EQ(rc, 0);

    /* payload too big */
    rc = spi_add_data_xfer_cmd(dev, &tx, &rx, MAX_TOTAL_PAYLOAD + 1);
    EXPECT_EQ(rc, ERR_TOO_BIG);

    /* also errors out because of the above error */
    rc = spi_add_cs_deassert_cmd(dev);
    EXPECT_LT(rc, 0);

    rc = spi_exec_cmds(dev, &failed);
    EXPECT_EQ(rc, ERR_BAD_STATE);
    /* failed index should point to data xfer command */
    EXPECT_EQ(failed, 1);
}

TEST_P(swspi, cs_double_assert) {
    int rc;
    size_t failed;
    struct spi_dev* dev = &_state->test_dev->dev;

    rc = spi_add_cs_assert_cmd(dev);
    EXPECT_EQ(rc, 0);

    /* should fail, because CS is already asserted */
    rc = spi_add_cs_assert_cmd(dev);
    EXPECT_EQ(rc, 0);

    rc = spi_exec_cmds(dev, &failed);
    EXPECT_LT(rc, 0);
    /* failed index should point to the second assert command */
    EXPECT_EQ(failed, 1);
}

TEST_P(swspi, cs_already_deasserted) {
    int rc;
    size_t failed;
    struct spi_dev* dev = &_state->test_dev->dev;

    /* should fail, because CS is already deasserted */
    rc = spi_add_cs_deassert_cmd(dev);
    EXPECT_EQ(rc, 0);

    rc = spi_exec_cmds(dev, &failed);
    EXPECT_LT(rc, 0);
    EXPECT_EQ(failed, 0);
}

TEST_P(swspi, cs_shared_bus) {
    int rc;
    size_t failed;
    struct spi_dev* dev = &_state->test_dev->dev;
    bool loopback = _state->test_dev->loopback;

    /*
     * Test device must be on a shared bus. That's not necessarily the case for
     * a loopback device.
     */
    if (loopback) {
        return;
    }

    /* swspi devices are on a shared bus, so CS can't be left asserted */
    rc = spi_add_cs_assert_cmd(dev);
    EXPECT_EQ(rc, 0);

    rc = spi_exec_cmds(dev, &failed);
    EXPECT_LT(rc, 0);
}

TEST_P(swspi, set_clk) {
    int rc;
    uint64_t* clk_hz = NULL;
    struct spi_dev* dev = &_state->test_dev->dev;

    rc = spi_add_set_clk_cmd(dev, CLK_SPEED, &clk_hz);
    EXPECT_EQ(rc, 0);

    rc = spi_exec_cmds(dev, NULL);
    EXPECT_EQ(rc, 0);

    EXPECT_LE(*clk_hz, CLK_SPEED);
}

TEST_P(swspi, single_data_xfer) {
    struct spi_test_dev* test_dev = _state->test_dev;
    int rc = exec_xfer(test_dev, 1);
    EXPECT_EQ(rc, 0);
}

TEST_P(swspi, short_single_data_xfer) {
    int rc;
    size_t max_size = 10;
    struct spi_test_dev* test_dev = _state->test_dev;

    /* execute short xfers */
    for (size_t i = 1; i <= max_size; i++) {
        rc = exec_xfer(test_dev, i);
        EXPECT_EQ(rc, 0, "size: %zu", i);
    }
}

TEST_P(swspi, long_single_data_xfer) {
    int rc;
    struct spi_test_dev* test_dev = _state->test_dev;

    /* execute longer xfer */
    rc = exec_xfer(test_dev, 256);
    EXPECT_EQ(rc, 0, "exec xfer 256");

    rc = exec_xfer(test_dev, 512);
    EXPECT_EQ(rc, 0, "exec xfer 512");

    rc = exec_xfer(test_dev, 1024);
    EXPECT_EQ(rc, 0, "exec xfer 1024");
}

TEST_P(swspi, multi_segment_xfer) {
    int rc;
    void* tx0;
    void* tx1;
    void* rx0;
    void* rx1;
    uint8_t expected0[128];
    uint8_t expected1[64];
    const size_t sz0 = sizeof(expected0);
    const size_t sz1 = sizeof(expected1);
    struct spi_dev* dev = &_state->test_dev->dev;
    bool loopback = _state->test_dev->loopback;

    rc = spi_add_cs_assert_cmd(dev);
    EXPECT_EQ(rc, 0);

    rc = spi_add_data_xfer_cmd(dev, &tx0, &rx0, sz0);
    EXPECT_EQ(rc, 0);

    rc = spi_add_data_xfer_cmd(dev, &tx1, &rx1, sz1);
    EXPECT_EQ(rc, 0);

    rc = spi_add_cs_deassert_cmd(dev);
    EXPECT_EQ(rc, 0);

    /* fill out TX buffers and expected RX buffers */
    rand_buf(tx0, sz0, 0);
    rand_buf(tx1, sz1, 1);
    rand_buf(expected0, sz0, digest(tx0, sz0));
    rand_buf(expected1, sz1, digest(tx1, sz1));

    rc = spi_exec_cmds(dev, NULL);
    EXPECT_EQ(rc, 0);

    /* check data */
    if (loopback) {
        EXPECT_EQ(memcmp(tx0, rx0, sz0), 0);
        EXPECT_EQ(memcmp(tx1, rx1, sz1), 0);
    } else {
        EXPECT_EQ(memcmp(expected0, rx0, sz0), 0);
        EXPECT_EQ(memcmp(expected1, rx1, sz1), 0);
    }
}

TEST_P(swspi, unidir_recv) {
    int rc;
    void* rx;
    uint8_t expected[TXRX_SIZE];
    size_t sz = sizeof(expected);
    struct spi_dev* dev = &_state->test_dev->dev;
    bool loopback = _state->test_dev->loopback;

    /* receive-only data transfers use seed 0 */
    rand_buf(expected, sz, 0);

    rc = spi_add_cs_assert_cmd(dev);
    EXPECT_EQ(rc, 0);

    rc = spi_add_data_xfer_cmd(dev, NULL, &rx, sz);
    EXPECT_EQ(rc, 0);

    rc = spi_add_cs_deassert_cmd(dev);
    EXPECT_EQ(rc, 0);

    rc = spi_exec_cmds(dev, NULL);
    EXPECT_EQ(rc, 0);

    /* check data */
    if (!loopback) {
        EXPECT_EQ(memcmp(expected, rx, sz), 0);
    }
}

INSTANTIATE_TEST_SUITE_P(swspi, swspi, testing_Range(0, SPI_DEV_COUNT));

PORT_TEST(swspi, "com.android.trusty.swspi.test");
