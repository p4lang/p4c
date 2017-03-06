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

/* The mother of all test programs that links with libp4ctoolkit.a and libfrontend.la */

#include <config.h>
#include <cstring>
#include <inttypes.h>
#include <iostream>
#include <limits.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include "lib/error.h"
#include "p4ctest.h"

using namespace Test;

TestDriver TestDriver::singleton;
static const char *program_name;

static void set_program_name(const char *argv0) {
    const char *slash = strrchr(argv0, '/');
    program_name = slash ? slash + 1 : argv0;
}

/*
 * Used to prevent the 'static initialization order fiasco'
 * https://isocpp.org/wiki/faq/ctors#static-init-order-on-first-use
 */
TestCommands& testCommands() {
    static TestCommands* cmds =new TestCommands();
    return *cmds;
}

void TestDriver::p4ctest_register(std::string test_name, CommandHandler f) {
    TestCommand test_cmd(test_name, f);
    TestDriver::add_command(test_cmd);
}

void TestDriver::add_command(TestCommand& cmd) {
    testCommands().push_back(cmd);
}

void TestDriver::add_top_level_commands() {
    TestCommand help_cmd("--help",
            [](TestCommandContext* ctx){
            printf("%s: the big test executable\n"
                    "usage: %s TEST [TESTARGS]\n"
                    "where TEST is one of the following. \n\n",
                    program_name, program_name);
            for (auto &cmd : testCommands()) {
                if (cmd.name[0] != '-') {
                    std::cout << cmd.name << std::endl;
            }}});

    TestDriver::add_command(help_cmd);
}

void TestDriver::p4c_cmdl_run_command(TestCommandContext* ctx, TestCommands& commands) {
    if (ctx->argc < 1) {
        std::cerr << "missing command name; use --help for help" << std::endl;
        exit(0);
    }
    for (auto &c : testCommands()) {
        if (c.name == ctx->argv[0]) {
            if (c.handler != nullptr) {
                c.handler(ctx);
                return;
    }}}
    std::cerr << "unknown command " << ctx->argv[0] << "; use --help for help" << std::endl;
}

/* Runs the command designated by argv[0] within the command table specified by
 * 'commands', which must be terminated by a command whose 'name' member is a
 * null pointer.
 *
 * Command-line options should be stripped off, so that a typical invocation
 * looks like:
 *    struct p4c_cmdl_context ctx = {
 *        .argc = argc - optind,
 *        .argv = argv + optind,
 *    };
 *    p4c_cmdl_run_command(&ctx, my_commands);
 * */
int main(int argc, char *argv[]) {
    set_program_name(argv[0]);

    if (argc < 2) {
        ::error("expect test program to be specified; use --help for usage");
        exit(0);
    }
    TestDriver::add_top_level_commands();
    if (argc > 1) {
        struct TestCommandContext ctx = {
            .argc = argc - 1,
            .argv = argv + 1,
        };
        TestDriver::p4c_cmdl_run_command(&ctx, testCommands());
    }

    return 0;
}
