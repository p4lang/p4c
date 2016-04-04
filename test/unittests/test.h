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
