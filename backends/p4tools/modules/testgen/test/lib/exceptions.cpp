// SPDX-FileCopyrightText: 2026 The P4 Language Consortium
//
// SPDX-License-Identifier: Apache-2.0

#include "backends/p4tools/modules/testgen/lib/exceptions.h"

#include <gtest/gtest.h>

#include "absl/strings/str_cat.h"

namespace P4::P4Tools::P4Testgen {

TEST(TestgenException, WithoutLocation) {
    const TestgenUnimplemented ex("test %1%", 42);
    EXPECT_EQ(ex.what(), absl::StrCat(Util::cerr_colorize(Util::ANSI_BLUE), "Not yet implemented",
                                      Util::cerr_clear_colors(), ":\ntest 42\n"));
}

TEST(TestgenException, CallerLocation) {
    const auto line = __LINE__ + 2;
    try {
        TESTGEN_UNIMPLEMENTED("test %1%", 42);
    } catch (const TestgenUnimplemented &ex) {
        EXPECT_EQ(ex.what(), absl::StrCat("In file: ", __FILE__, ":", line, "\n",
                                          Util::cerr_colorize(Util::ANSI_BLUE),
                                          "Unimplemented compiler support",
                                          Util::cerr_clear_colors(), ": test 42\n"));
        return;
    }
    FAIL() << "TESTGEN_UNIMPLEMENTED did not throw";
}

}  // namespace P4::P4Tools::P4Testgen
