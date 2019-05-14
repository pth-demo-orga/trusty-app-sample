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

#include <endian.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <trusty_unittest.h>

#define CHECK_ERRNO(e)       \
    do {                     \
        ASSERT_EQ(e, errno); \
        errno = 0;           \
    } while (0)
#define CLEAR_ERRNO() \
    do {              \
        errno = 0;    \
    } while (0)

typedef struct libc {
} libc_t;

TEST_F_SETUP(libc) {
    /* Isolate the tests. */
    CLEAR_ERRNO();
}

TEST_F_TEARDOWN(libc) {
    /* errno should have been checked and cleared if the test sets errno. */
    CHECK_ERRNO(0);

test_abort:;
}

/*
 * Smoke test to make sure the endian functions are defined.
 * Musl may or may not expose them, depending on the feature test macros.
 */
TEST_F(libc, endian) {
    const uint32_t test_data = 0x12345678;
    /* TODO test le32, etc, once they are provided. */
    ASSERT_EQ(test_data, be32toh(htobe32(test_data)));
test_abort:;
}

TEST_F(libc, memset_test) {
    char buf[130];
    buf[0] = 0;
    buf[129] = 0;
    for (int val = 1; val < 256; val <<= 1) {
        memset(&buf[1], val, 128);
        ASSERT_EQ(0, buf[0], "iteration %d", val);
        for (unsigned int i = 1; i < 128; i++) {
            ASSERT_EQ(val, buf[i], "iteration %d", val);
        }
        ASSERT_EQ(0, buf[129], "iteration %d", val);
    }

test_abort:;
}

TEST_F(libc, memcmp_test) {
    unsigned char buf1[128];
    unsigned char buf2[128];

    /* Identical buffers. */
    memset(buf1, 7, sizeof(buf1));
    memset(buf2, 7, sizeof(buf2));
    ASSERT_EQ(0, memcmp(buf1, buf2, sizeof(buf1)));

    /* buf1 slightly greater. */
    buf1[127] = 9;
    buf2[127] = 8;
    ASSERT_LT(0, memcmp(buf1, buf2, sizeof(buf1)));

    /* buf1 much greater. */
    buf1[127] = 127;
    buf2[127] = 0;
    ASSERT_LT(0, memcmp(buf1, buf2, sizeof(buf1)));

    /* buf2 slightly greater. */
    buf1[127] = 8;
    buf2[127] = 9;
    ASSERT_GT(0, memcmp(buf1, buf2, sizeof(buf1)));

    /* buf2 much greater. */
    buf1[127] = 0;
    buf2[127] = 127;
    ASSERT_GT(0, memcmp(buf1, buf2, sizeof(buf1)));

    /* Sanity check, make them identical again. */
    memcpy(buf2, buf1, sizeof(buf1));
    ASSERT_EQ(0, memcmp(buf1, buf2, sizeof(buf1)));

test_abort:;
}

TEST_F(libc, strcmp_test) {
    ASSERT_EQ(0, strcmp("", ""));
    ASSERT_GT(0, strcmp("", "bar"));
    ASSERT_LT(0, strcmp("bar", ""));

    ASSERT_EQ(0, strcmp("bar", "bar"));
    ASSERT_GT(0, strcmp("bar", "baz"));
    ASSERT_LT(0, strcmp("baz", "bar"));

    ASSERT_GT(0, strcmp("bar", "barbar"));
    ASSERT_LT(0, strcmp("barbar", "bar"));

    char negative[2] = {-127, 0};
    char positive[2] = {0, 0};
    // strcmp must treat characters as unsigned
    ASSERT_LT(0, strcmp(negative, positive));
    ASSERT_LT(0, strncmp(negative, positive, 1));

test_abort:;
}

#define MSEC 1000000ULL

/*
 * Smoke test the time-related functions.
 * As long as gettime and nanosleep behave semi-reasonablly, we're happy.
 */
TEST_F(libc, time) {
    int64_t begin = 0;
    int64_t end = 0;
    int64_t delta = 0;

    trusty_gettime(0, &begin);
    trusty_nanosleep(0, 0, 10 * MSEC);
    trusty_gettime(0, &end);
    delta = end - begin;

    ASSERT_LT(1 * MSEC, delta);
    /* We've observed 200 ms sleeps in the emulator, so be generous. */
    ASSERT_LT(delta, 1000 * MSEC);

test_abort:;
}

TEST_F(libc, snprintf_test) {
    char buffer[16];
    ASSERT_EQ(17, snprintf(buffer, sizeof(buffer), "%d %x %s...", 12345, 254,
                           "hello"));
    ASSERT_EQ(0, strcmp(buffer, "12345 fe hello."));

test_abort:;
}

TEST_F(libc, atoi_test) {
    ASSERT_EQ(12345, atoi("12345"));
    ASSERT_EQ(-67890, atoi("-67890"));
    /* Note: Out-of-bound values are undefined behavior. */

test_abort:;
}

TEST_F(libc, print_test) {
    /*
     * Test printing compiles and doesn't crash. Yes, this is a weak test.
     * A stronger test would be better, but also more complicated. Stay simple,
     * for now.
     */
    printf("Hello, stdout.\n");
    fprintf(stderr, "Hello, stderr.\n");
    CHECK_ERRNO(0);

test_abort:;
}

TEST_F(libc, print_float_test) {
    /*
     * %f should be valid and not cause an error, even if floating point
     * support is disabled.
     */
    printf("num: %f\n", 1.23);
    CHECK_ERRNO(0);

test_abort:;
}

TEST_F(libc, print_errno_test) {
    /*
     * %m is not supported, but should not be an error, either.
     */
    printf("err: %m\n");
    CHECK_ERRNO(0);

test_abort:;
}

TEST_F(libc, print_bad_test) {
    printf("[%k]\n");
    /* TODO: EINVAL */
    CLEAR_ERRNO();

test_abort:;
}

TEST_F(libc, malloc_loop) {
    for (int i = 0; i < 1024; i++) {
        void* ptr = malloc(4096 * 3);
        ASSERT_NE(0, ptr, "iteration %d", i);
        free(ptr);
    }

test_abort:;
}

TEST_F(libc, malloc_oom) {
    void* ptr = malloc(8192 * 1024);
    ASSERT_EQ(0, ptr);
    /* TODO: ENOMEM */
    CLEAR_ERRNO();

test_abort:;
}

static uintptr_t expected_malloc_alignment(size) {
    /* TODO use ffs? */
    if (size >= 16) {
        return sizeof(void*) * 2;
    } else if (size >= 8) {
        return 8;
    } else if (size >= 4) {
        return 4;
    } else if (size >= 2) {
        return 2;
    } else {
        return 1;
    }
}

TEST_F(libc, malloc_alignment) {
    for (int size = 2; size < 256; size++) {
        const uintptr_t alignment_mask = expected_malloc_alignment(size) - 1;
        void* ptr1 = malloc(size);
        void* ptr2 = malloc(size / 2); /* Try to shake up the alignment. */
        void* ptr3 = malloc(size);

        ASSERT_EQ(0, (uintptr_t)ptr1 & alignment_mask, "size %d / align %ld",
                  size, alignment_mask + 1);
        ASSERT_EQ(0, (uintptr_t)ptr3 & alignment_mask, "size %d / align %ld",
                  size, alignment_mask + 1);

        free(ptr3);
        free(ptr2);
        free(ptr1);
    }
test_abort:;
}

/*
 * Grab the frame pointer in a simple, non-inlined function.
 * Note this isn't a static function. We're trying to game the optimizer and
 * ensure it doesn't change the calling convention.
 */
__attribute__((__noinline__)) uintptr_t frame_ptr() {
    return (uintptr_t)__builtin_frame_address(0);
}

TEST_F(libc, stack_alignment) {
    /*
     * On all the platforms we support, the frame pointer should be aligned to 2
     * times pointer size. This includes x86_64 because the stack pointer is
     * implicitly re-aligned after function entry before it becomes the frame
     * pointer.
     * Note that this test passing does not guarentee correctness, but it can
     * catch badness.
     */
    const uintptr_t alignment_mask = sizeof(void*) * 2 - 1;
    ASSERT_EQ(0, frame_ptr() & alignment_mask);

test_abort:;
}

#if __ARM_NEON__ || __ARM_NEON

#include <arm_neon.h>

/*
 * NOTE this is a fairly weak test that checks if a neon instruction can be
 * executed. This will help detect cases where the build flags do not match the
 * actual system the code is running on.
 */
TEST_F(libc, basic_neon) {
    int8x16_t block1 = vdupq_n_u8(0x55);
    int8x16_t block2 = vdupq_n_u8(0x33);
    int8x16_t expected;
    int8x16_t result;

    /* memset just to be sure. */
    memset(&expected, 0x66, sizeof(int8x16_t));

    result = veorq_s8(block1, block2);
    ASSERT_EQ(0, memcmp(&expected, &result, sizeof(int8x16_t)));

test_abort:;
}

#endif

PORT_TEST(libc, "com.android.libctest");
