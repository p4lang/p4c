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

// Processes --version, which prints the version information and dependency metadata, then exits.
void processVersionOption() {
    CompilerOptions options;
    options.compilerVersion = "4.5.6"_cs;
    const char *argv[] = {"gtestp4c", "--version"};
    options.process(2, const_cast<char *const *>(argv));
}

}  // namespace

class VersionOption : public P4CTest {};

// The first two lines are the binary name and compiler version. Every line after them has the
// form "dependency name=<name> version=<version> revision=<revision>".
TEST_F(VersionOption, ListsVersionAndLinkedLibraries) {
    EXPECT_EXIT(processVersionOption(), ::testing::ExitedWithCode(0),
                "^gtestp4c\nVersion 4[.]5[.]6\n"
                "(dependency name=[a-zA-Z0-9_-]+ version=[^ \t\r\n]+ revision=[^ \t\r\n]+\n)+$");
}

}  // namespace P4::Test
