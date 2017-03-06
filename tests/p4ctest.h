/*
 * Copyright (c) 2014 Nicira, Inc.
 * Copyright (c) 2013-present Barefoot Networks, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef P4CTEST_H
#define P4CTEST_H

#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>
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

struct TestCommandContext {
    /* number of command line arguments */
    int argc;
    /* array of command line arguments */
    char **argv;
    /* private context data defined by the API user */
    void *pvt;
};

typedef std::function<void(TestCommandContext* ctx)> CommandHandler;

class TestCommand {
 public:
    TestCommand(std::string name, CommandHandler handler) : name(name), handler(handler) {}
    CommandHandler handler;
    std::string name;
};

typedef std::vector<TestCommand> TestCommands;

class TestDriver {
 static TestDriver singleton;
 public:
    static TestCommands commands;
    TestDriver() {}
    static void add_command(TestCommand& cmd);
    static void add_top_level_commands();
    static void p4c_cmdl_run_command(TestCommandContext* ctx, TestCommands& commands);
    static void p4ctest_register(std::string test_name, CommandHandler handler);
};

}  // namespace Test

/* Overview
 * ========
 *
 * With p4ctest, each test programs is a sub program of p4ctest.
 * For example, 'mytest' program, can now be invoked as 'p4ctest mytest'.
 *
 * 'p4ctest --help' will list all test programs can be invoked.
 *
 * The 'Usage' section below documents how to add a new sub program
 * to p4ctest using P4CTEST_REIGSTER macros.
 */

/* Usage
 * =====
 *
 * For each sub test program, its 'main' program should be named as
 * '<test_name>_main()'.
 *
 * The 'main' programs should be registered with p4ctest as a sub program.
 *    P4CTEST_REGISTER(name, function)
 *
 * 'name' will be name of the test program. It is expected as argv[1] when
 * invoking with p4ctest.
 *
 * 'function' is the 'main' program mentioned above.
 *
 * Example:
 * ----------
 *
 * Suppose the test program is called my-test.c
 * ...
 *
 * static void
 * my_test_main(int argc, char *argv[])
 * {
 *   ....
 * }
 *
 * P4CTEST_REGISTER("my-test", my_test_main);
 */

#define P4C_CONSTRUCTOR(f) \
    static void f(void) __attribute__((constructor)); \
    static void f(void)

#define P4CTEST_REGISTER(name, function) \
    static void \
    p4ctest_wrapper_##function##__(Test::TestCommandContext *ctx) \
    { \
        function(ctx->argc, ctx->argv); \
    } \
    P4C_CONSTRUCTOR(register_##function) { \
        Test::TestDriver::p4ctest_register(name, p4ctest_wrapper_##function##__); \
    }

#endif  /* P4CTEST_H_ */

