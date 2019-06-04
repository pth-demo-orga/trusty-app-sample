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

#include <memory>
#include <string>

#include <trusty_unittest.h>

int global_count;

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
    global_count = 0;
}

TEST_F_TEARDOWN(libcxx) {
    /* errno should have been checked and cleared if the test sets errno. */
    CHECK_ERRNO(0);
    ASSERT_EQ(0, global_count);

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

class Counter {
public:
    Counter() { global_count++; }

    ~Counter() { global_count--; }
};

TEST_F(libcxx, unique_ptr) {
    ASSERT_EQ(0, global_count);
    {
        std::unique_ptr<Counter> u(new Counter());
        ASSERT_EQ(1, global_count);
    }
    ASSERT_EQ(0, global_count);
test_abort:;
}

TEST_F(libcxx, unique_ptr_move) {
    Counter* p = new Counter();
    std::unique_ptr<Counter> a(p);
    std::unique_ptr<Counter> b;

    ASSERT_EQ(1, global_count);
    ASSERT_EQ(p, a.get());
    ASSERT_EQ(nullptr, b.get());

    b = std::move(a);

    ASSERT_EQ(1, global_count);
    ASSERT_EQ(nullptr, a.get());
    ASSERT_EQ(p, b.get());

    b.reset();
    ASSERT_EQ(0, global_count);
    ASSERT_EQ(nullptr, b.get());

test_abort:;
}

TEST_F(libcxx, shared_ptr) {
    std::shared_ptr<Counter> a;
    std::shared_ptr<Counter> b;
    ASSERT_EQ(0, global_count);
    a.reset(new Counter());
    ASSERT_EQ(1, global_count);
    b = a;
    ASSERT_EQ(1, global_count);
    ASSERT_NE(nullptr, a.get());
    ASSERT_EQ(a.get(), b.get());
    a.reset();
    ASSERT_EQ(1, global_count);
    b.reset();
    ASSERT_EQ(0, global_count);

test_abort:;
}

TEST_F(libcxx, shared_ptr_move) {
    Counter* p = new Counter();
    std::shared_ptr<Counter> a(p);
    std::shared_ptr<Counter> b;

    ASSERT_EQ(1, global_count);
    ASSERT_EQ(p, a.get());
    ASSERT_EQ(nullptr, b.get());

    b = std::move(a);

    ASSERT_EQ(1, global_count);
    ASSERT_EQ(nullptr, a.get());
    ASSERT_EQ(p, b.get());

    b.reset();
    ASSERT_EQ(0, global_count);
    ASSERT_EQ(nullptr, b.get());

test_abort:;
}

TEST_F(libcxx, weak_ptr) {
    std::weak_ptr<Counter> w;
    ASSERT_EQ(0, global_count);
    {
        std::shared_ptr<Counter> s(new Counter());
        w = s;
        ASSERT_EQ(1, global_count);
        ASSERT_EQ(1, w.use_count());
        {
            auto t = w.lock();
            ASSERT_EQ(1, global_count);
            ASSERT_EQ(2, w.use_count());
            ASSERT_EQ(s.get(), t.get());
        }
        ASSERT_EQ(1, global_count);
        ASSERT_EQ(1, w.use_count());
    }
    ASSERT_EQ(0, global_count);
    ASSERT_EQ(0, w.use_count());

test_abort:;
}

TEST_F(libcxx, weak_ptr_move) {
    std::shared_ptr<Counter> s(new Counter());
    std::weak_ptr<Counter> a(s);
    std::weak_ptr<Counter> b;

    ASSERT_EQ(1, global_count);
    ASSERT_EQ(1, a.use_count());
    ASSERT_EQ(0, b.use_count());

    b = std::move(a);

    ASSERT_EQ(1, global_count);
    ASSERT_EQ(0, a.use_count());
    ASSERT_EQ(1, b.use_count());

    s.reset();

    ASSERT_EQ(0, global_count);
    ASSERT_EQ(0, b.use_count());

test_abort:;
}

// TODO test framework does not compare anything that can't be cast to long.
TEST_F(libcxx, string_append) {
    std::string a("abcdefghijklmnopqrstuvwxyz!!!");
    std::string b("abcdefghijklmnopqrstuvwxyz");
    ASSERT_NE(0, strcmp(a.c_str(), b.c_str()));
    b += "!!!";
    ASSERT_EQ(0, strcmp(a.c_str(), b.c_str()));

test_abort:;
}

TEST_F(libcxx, string_move) {
    std::string a("foo");
    std::string b;
    ASSERT_EQ(0, strcmp(a.c_str(), "foo"));
    ASSERT_NE(0, strcmp(b.c_str(), "foo"));

    b = std::move(a);

    ASSERT_NE(0, strcmp(a.c_str(), "foo"));
    ASSERT_EQ(0, strcmp(b.c_str(), "foo"));

test_abort:;
}

TEST_F(libcxx, to_string) {
    ASSERT_EQ(0, strcmp(std::to_string(123).c_str(), "123"));

test_abort:;
}

PORT_TEST(libcxx, "com.android.libcxxtest");
