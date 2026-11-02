// SPDX-FileCopyrightText: 2026 The P4 Language Consortium
//
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include "frontends/common/options.h"
#include "frontends/common/parser_options.h"
#include "lib/cstring.h"
#include "test/gtest/helpers.h"

namespace P4::Test {

using namespace P4::literals;

namespace {

// Registers a library and processes --version, which prints the version information and exits.
// The options are created here because the --version handler is bound to the object that
// registered it, which is not the instance that GTestContext hands out.
void processVersionOption() {
    ParserOptions::addLinkedLibrary("TestLibrary", "1.2.3");
    CompilerOptions options;
    options.compilerVersion = "4.5.6"_cs;
    const char *argv[] = {"gtestp4c", "--version"};
    options.process(2, const_cast<char *const *>(argv));
}

}  // namespace

class VersionOption : public P4CTest {};

// The first two lines are the binary name and the version. Every line after them has the form
// "<library> <version>". The exact set of libraries depends on how the binary was built, so only
// the library registered by the test is required.
TEST_F(VersionOption, ListsVersionAndLinkedLibraries) {
    EXPECT_EXIT(processVersionOption(), ::testing::ExitedWithCode(0),
                "^gtestp4c\nVersion 4[.]5[.]6\n"
                "([A-Za-z0-9]+ [0-9][0-9.]*\n)*"
                "TestLibrary 1[.]2[.]3\n"
                "([A-Za-z0-9]+ [0-9][0-9.]*\n)*$");
}

}  // namespace P4::Test
