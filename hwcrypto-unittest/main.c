/*
 * Copyright (C) 2015 The Android Open Source Project
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

/*
 * Tests:
 * generic:
 * - no session / invalid session
 * - closed session
 *
 * hwkey:
 * - derive twice to same result
 * - derive different, different result
 * - keyslot, invalid slot
 *
 * rng:
 *
 */

#define TLOG_TAG "hwcrypto_unittest"

#include <stdlib.h>
#include <string.h>

#include <lib/hwkey/hwkey.h>
#include <lib/rng/trusty_rng.h>
#include <trusty_unittest.h>
#include <uapi/err.h>

#define RPMB_STORAGE_AUTH_KEY_ID "com.android.trusty.storage_auth.rpmb"
#define HWCRYPTO_UNITTEST_KEYBOX_ID "com.android.trusty.hwcrypto.unittest.key32"

#define STORAGE_AUTH_KEY_SIZE 32

#if WITH_HWCRYPTO_UNITTEST
static const uint8_t UNITTEST_KEYSLOT[] = "unittestkeyslotunittestkeyslotun";
#else
#pragma message                                                                          \
        "hwcrypto-unittest is built with the WITH_HWCRYPTO_UNITTEST define not enabled." \
        "Hwkey tests will not test anything."
#endif

/*
 * Implement this hook for device specific hwkey tests
 */
__WEAK void run_device_hwcrypto_unittest(void) {}

TEST(hwcrypto, device_hwcrypto_unittest) {
    run_device_hwcrypto_unittest();
}

typedef struct hwkey {
    hwkey_session_t hwkey_session;
} hwkey_t;

TEST_F_SETUP(hwkey) {
    int rc;

    _state->hwkey_session = INVALID_IPC_HANDLE;
    rc = hwkey_open();
    ASSERT_GE(rc, 0);
    _state->hwkey_session = (hwkey_session_t)rc;

test_abort:;
}

TEST_F_TEARDOWN(hwkey) {
    close(_state->hwkey_session);
}

TEST_F(hwkey, generic_invalid_session) {
    const uint8_t src_data[] = "thirtytwo-bytes-of-nonsense-data";
    static const size_t size = sizeof(src_data);
    uint8_t dest[sizeof(src_data)];

    hwkey_session_t invalid = INVALID_IPC_HANDLE;
    uint32_t kdf_version = HWKEY_KDF_VERSION_BEST;

    // should fail immediately
    long rc = hwkey_derive(invalid, &kdf_version, src_data, dest, size);
    EXPECT_EQ(ERR_BAD_HANDLE, rc, "generic - bad handle");
}

TEST_F(hwkey, generic_closed_session) {
    static const uint8_t src_data[] = "thirtytwo-bytes-of-nonsense-data";
    static const uint32_t size = sizeof(src_data);
    uint8_t dest[sizeof(src_data)];
    uint32_t kdf_version = HWKEY_KDF_VERSION_BEST;

    long rc = hwkey_open();
    EXPECT_GE(rc, 0, "generic - open");

    hwkey_session_t session = (hwkey_session_t)rc;
    hwkey_close(session);

    // should fail immediately
    rc = hwkey_derive(session, &kdf_version, src_data, dest, size);
    EXPECT_EQ(ERR_NOT_FOUND, rc, "generic - closed handle");
}

TEST_F(hwkey, derive_repeatable) {
    const uint8_t src_data[] = "thirtytwo-bytes-of-nonsense-data";
    uint8_t dest[32];
    uint8_t dest2[sizeof(dest)];
    static const size_t size = sizeof(dest);
    uint32_t kdf_version = HWKEY_KDF_VERSION_BEST;

    memset(dest, 0, size);
    memset(dest2, 0, size);

    /* derive key once */
    long rc = hwkey_derive(_state->hwkey_session, &kdf_version, src_data, dest,
                           size);
    EXPECT_EQ(NO_ERROR, rc, "derive repeatable - initial derivation");
    EXPECT_NE(HWKEY_KDF_VERSION_BEST, kdf_version,
              "derive repeatable - kdf version");

    /* derive key again */
    rc = hwkey_derive(_state->hwkey_session, &kdf_version, src_data, dest2,
                      size);
    EXPECT_EQ(NO_ERROR, rc, "derive repeatable - second derivation");

    /* ensure they are the same */
    rc = memcmp(dest, dest2, size);
    EXPECT_EQ(0, rc, "derive repeatable - equal");
    rc = memcmp(dest, src_data, size);
    EXPECT_NE(0, rc, "derive repeatable - same as seed");
}

TEST_F(hwkey, derive_different) {
    const uint8_t src_data[] = "thirtytwo-bytes-of-nonsense-data";
    const uint8_t src_data2[] = "thirtytwo-byt3s-of-nons3ns3-data";

    uint8_t dest[32];
    uint8_t dest2[sizeof(dest)];
    static const uint32_t size = sizeof(dest);
    uint32_t kdf_version = HWKEY_KDF_VERSION_BEST;

    memset(dest, 0, size);
    memset(dest2, 0, size);

    /* derive key once */
    long rc = hwkey_derive(_state->hwkey_session, &kdf_version, src_data, dest,
                           size);
    EXPECT_EQ(NO_ERROR, rc, "derive not repeatable - initial derivation");
    EXPECT_NE(HWKEY_KDF_VERSION_BEST, kdf_version,
              "derive not repeatable - kdf version");

    /* derive key again, with different source data */
    rc = hwkey_derive(_state->hwkey_session, &kdf_version, src_data2, dest2,
                      size);
    EXPECT_EQ(NO_ERROR, rc, "derive not repeatable - second derivation");

    /* ensure they are not the same */
    rc = memcmp(dest, dest2, size);
    EXPECT_NE(0, rc, "derive not repeatable - equal");
    rc = memcmp(dest, src_data, size);
    EXPECT_NE(0, rc, "derive not repeatable - equal to source");
    rc = memcmp(dest2, src_data2, size);
    EXPECT_NE(0, rc, "derive not repeatable - equal to source");
}

TEST_F(hwkey, derive_zero_length) {
    static const uint32_t size = 0;
    const uint8_t* src_data = NULL;
    uint8_t* dest = NULL;
    uint32_t kdf_version = HWKEY_KDF_VERSION_BEST;

    /* derive key once */
    long rc = hwkey_derive(_state->hwkey_session, &kdf_version, src_data, dest,
                           size);
    EXPECT_EQ(ERR_NOT_VALID, rc, "derive zero length");
}

TEST_F(hwkey, get_storage_auth) {
    uint32_t actual_size = STORAGE_AUTH_KEY_SIZE;
    uint8_t storage_auth_key[STORAGE_AUTH_KEY_SIZE];
    long rc = hwkey_get_keyslot_data(_state->hwkey_session,
                                     RPMB_STORAGE_AUTH_KEY_ID, storage_auth_key,
                                     &actual_size);
    EXPECT_EQ(ERR_NOT_FOUND, rc, "auth key accessible when it shouldn't be");
}

TEST_F(hwkey, get_keybox) {
    uint8_t dest[sizeof(HWCRYPTO_UNITTEST_KEYBOX_ID)];
    uint32_t actual_size = sizeof(dest);
    long rc = hwkey_get_keyslot_data(_state->hwkey_session,
                                     HWCRYPTO_UNITTEST_KEYBOX_ID, dest,
                                     &actual_size);

#if WITH_HWCRYPTO_UNITTEST
    EXPECT_EQ(NO_ERROR, rc, "get hwcrypto-unittest keybox");
    rc = memcmp(UNITTEST_KEYSLOT, dest, sizeof(UNITTEST_KEYSLOT) - 1);
    EXPECT_EQ(0, rc, "get storage auth key invalid");
#else
    EXPECT_EQ(ERR_NOT_FOUND, rc, "get hwcrypto-unittest keybox");
#endif
}

/***********************   HWRNG  UNITTEST  ***********************/

static uint32_t _hist[256];
static uint8_t _rng_buf[1024];

static void hwrng_update_hist(uint8_t* data, unsigned int cnt) {
    for (unsigned int i = 0; i < cnt; i++) {
        _hist[data[i]]++;
    }
}

static void hwrng_show_data(const void* ptr, size_t len) {
    uintptr_t address = (uintptr_t)ptr;
    size_t count;
    size_t i;
    fprintf(stderr, "Dumping first hwrng request:\n");
    for (count = 0; count < len; count += 16) {
        for (i = 0; i < MIN(len - count, 16); i++) {
            fprintf(stderr, "0x%02hhx ", *(const uint8_t*)(address + i));
        }
        fprintf(stderr, "\n");
        address += 16;
    }
}

TEST(hwrng, show_data_test) {
    int rc;
    rc = trusty_rng_hw_rand(_rng_buf, 32);
    EXPECT_EQ(NO_ERROR, rc, "hwrng test");
    if (rc == NO_ERROR) {
        hwrng_show_data(_rng_buf, 32);
    }
}

TEST(hwrng, var_rng_req_test) {
    int rc;
    unsigned int i;
    size_t req_cnt;
    /* Issue 100 hwrng requests of variable sizes */
    for (i = 0; i < 100; i++) {
        req_cnt = ((size_t)rand() % sizeof(_rng_buf)) + 1;
        rc = trusty_rng_hw_rand(_rng_buf, req_cnt);
        EXPECT_EQ(NO_ERROR, rc, "hwrng test");
        if (rc != NO_ERROR) {
            TLOGI("trusty_rng_hw_rand returned %d\n", rc);
            continue;
        }
    }
}

TEST(hwrng, stats_test) {
    int rc;
    unsigned int i;
    size_t req_cnt;
    uint32_t exp_cnt;
    uint32_t cnt = 0;
    uint32_t ave = 0;
    uint32_t dev = 0;
    /* issue 100x256 bytes requests */
    req_cnt = 256;
    exp_cnt = 1000 * req_cnt;
    memset(_hist, 0, sizeof(_hist));
    for (i = 0; i < 1000; i++) {
        rc = trusty_rng_hw_rand(_rng_buf, req_cnt);
        EXPECT_EQ(NO_ERROR, rc, "hwrng test");
        if (rc != NO_ERROR) {
            TLOGI("trusty_rng_hw_rand returned %d\n", rc);
            continue;
        }
        hwrng_update_hist(_rng_buf, req_cnt);
    }

    /* check hwrng stats */
    for (i = 0; i < 256; i++)
        cnt += _hist[i];
    ave = cnt / 256;
    EXPECT_EQ(exp_cnt, cnt, "hwrng ttl sample cnt");
    EXPECT_EQ(1000, ave, "hwrng eve sample cnt");

    /*
     * Ideally data should be uniformly distributed
     * Calculate average deviation from ideal model
     */
    for (i = 0; i < 256; i++) {
        uint32_t val = (_hist[i] > ave) ? _hist[i] - ave : ave - _hist[i];
        dev += val;
    }
    dev /= 256;
    /*
     * Check if average deviation is within 5% of ideal model
     * which is fairly arbitrary requirement. It could be useful
     * to alert is something terribly wrong with rng source.
     */
    EXPECT_GT(50, dev, "average dev");
}

PORT_TEST(hwcrypto, "com.android.trusty.hwcrypto.test")
