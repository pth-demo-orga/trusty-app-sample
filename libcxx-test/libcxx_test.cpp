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

#include <errno.h>

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

typedef struct libcxx {
} libcxx_t;

TEST_F_SETUP(libcxx) {
    /* Isolate the tests. */
    CLEAR_ERRNO();
}

TEST_F_TEARDOWN(libcxx) {
    /* errno should have been checked and cleared if the test sets errno. */
    CHECK_ERRNO(0);

test_abort:;
}

class Dummy {};

TEST_F(libcxx, new_and_delete) {
    Dummy* tmp = new Dummy();
    ASSERT_NE(nullptr, tmp);
    delete tmp;
test_abort:;
}

/*
 * Inspecting the generated code, it appears this variable can be optimized out
 * if it is not declared volatile.
 */
volatile bool did_init;

class GlobalSetter {
public:
    GlobalSetter() { did_init = true; }
};

GlobalSetter setter;

TEST_F(libcxx, global_constructor) {
    /* Did a global constructor run? */
    ASSERT_EQ(true, did_init);
test_abort:;
}

PORT_TEST(libcxx, "com.android.libcxxtest");
