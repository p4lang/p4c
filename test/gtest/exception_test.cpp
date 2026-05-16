// SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
// Copyright 2013-present Barefoot Networks, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include <exception>

#include "lib/cstring.h"
#include "lib/exceptions.h"

namespace P4::Util {

TEST(UtilException, Messages) {
    // Check that exception message formatting works as expected.
    try {
        throw CompilerBug("test");
    } catch (std::exception &ex) {
        cstring err(ex.what());
        cstring redir_msg("Compiler Bug:\ntest\n");
        cstring no_redir_msg = cstring(ANSI_RED) + "Compiler Bug" + ANSI_CLR + ":\ntest\n";
        // The error message might or might not be colorized based on if the test are redirected
        // or not, to make sure both options are valid an array is used.
        bool is_content_correct = (err == redir_msg) || (err == no_redir_msg);
        EXPECT_EQ(is_content_correct, true);
    }

    try {
        throw CompilationError("Testing error %1%", 1);
    } catch (std::exception &ex) {
        cstring err(ex.what());
        EXPECT_EQ(err, "Testing error 1\n");
    }
}

}  // namespace P4::Util
