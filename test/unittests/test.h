/*
Copyright 2013-present Barefoot Networks, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

/* -*-C++-*- */

#ifndef P4C_TEST_UNITTESTS_TEST_H_
#define P4C_TEST_UNITTESTS_TEST_H_

#include <iostream>
#include <cassert>
#include "lib/stringify.h"

#define ASSERT_EQ(a, b) \
    do {                  \
        if ((a) != (b)) {                                               \
            std::clog << cstring("") << #a << " is `" << Util::toString(a) << "'" << std::endl \
                      << cstring("") << #b << " is `" << Util::toString(b) << "'" << std::endl; \
            assert((a) == (b));                                         \
        }                                                               \
    } while (0)

#define ASSERT_NEQ(a, b) \
    do {                  \
        if ((a) == (b)) {                                               \
            std::clog << cstring("") << #a << " is `" << Util::toString(a) << "'" << std::endl \
                      << cstring("") << #b << " is `" << Util::toString(b) << "'" << std::endl; \
            assert((a) != (b));                                         \
        }                                                               \
    } while (0)

#define UNREACHABLE()                           \
    do {                                                   \
        std::clog << "Should not reach here" << std::endl; \
        assert(false);                                     \
    } while (0)

#define RUNTEST(f)  do {  \
        int result = f();                     \
        if (result != SUCCESS) return result; \
    } while (0)

namespace Test {
class TestBase {
 public:
    virtual ~TestBase() {}
    virtual int run() = 0;

    const int SUCCESS = 0;
    const int FAILURE = 1;
};
}  // namespace Test

#endif /* P4C_TEST_UNITTESTS_TEST_H_ */
