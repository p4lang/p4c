// SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
// Copyright 2013-present Barefoot Networks, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include <exception>
#include <memory>
#include <string>

#include "absl/strings/str_cat.h"
#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"
#include "lib/null.h"
#include "lib/source_file.h"

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

TEST(UtilException, PreformattedBugMessage) {
    const CompilerBug ex(std::string("Already formatted: %1% %s\n"));
    EXPECT_EQ(ex.what(), absl::StrCat(cerr_colorize(ANSI_RED), "Compiler Bug", cerr_clear_colors(),
                                      ":\nAlready formatted: %1% %s\n"));
}

TEST(UtilException, UnimplementedWithoutLocation) {
    const CompilerUnimplemented ex("test %1%", 42);
    EXPECT_EQ(ex.what(), absl::StrCat(cerr_colorize(ANSI_BLUE), "Not yet implemented",
                                      cerr_clear_colors(), ":\ntest 42\n"));
}

TEST(UtilException, UnimplementedEntriesSourceLocation) {
    InputSources sources;
    sources.mapLine("entries.p4", 1);
    sources.appendText("const entries = {};\n");
    sources.seal();
    const SourceInfo info(&sources, SourcePosition(1, 0), SourcePosition(1, 19));
    const IR::EntriesList entries(info, IR::Vector<IR::Entry>{});

    const auto line = __LINE__ + 2;
    try {
        P4C_UNIMPLEMENTED("Table entries are not yet implemented in this backend: %1%", &entries);
    } catch (const CompilerUnimplemented &ex) {
        const std::string message(ex.what());
        EXPECT_NE(message.find(absl::StrCat("In file: ", __FILE__, ":", line, "\n")),
                  std::string::npos);
        EXPECT_NE(message.find(info.toPositionString().c_str()), std::string::npos);
        EXPECT_NE(message.find(info.toSourceFragment()), std::string::npos);
        EXPECT_NE(message.find("Table entries are not yet implemented in this backend"),
                  std::string::npos);
        return;
    }
    FAIL() << "P4C_UNIMPLEMENTED did not throw";
}

TEST(UtilException, BugCallerLocation) {
    const auto line = __LINE__ + 2;
    try {
        BUG("test %1%", 42);
    } catch (const CompilerBug &ex) {
        EXPECT_EQ(ex.what(),
                  absl::StrCat("In file: ", __FILE__, ":", line, "\n", cerr_colorize(ANSI_RED),
                               "Compiler Bug", cerr_clear_colors(), ": test 42\n"));
        return;
    }
    FAIL() << "BUG did not throw";
}

TEST(UtilException, UnimplementedCallerLocation) {
    const auto line = __LINE__ + 2;
    try {
        P4C_UNIMPLEMENTED("test %s", "feature");
    } catch (const CompilerUnimplemented &ex) {
        EXPECT_EQ(ex.what(),
                  absl::StrCat("In file: ", __FILE__, ":", line, "\n", cerr_colorize(ANSI_BLUE),
                               "Unimplemented compiler support", cerr_clear_colors(),
                               ": test feature\n"));
        return;
    }
    FAIL() << "P4C_UNIMPLEMENTED did not throw";
}

TEST(UtilException, FatalError) {
    try {
        FATAL_ERROR("test %1%", 42);
    } catch (const CompilationError &ex) {
        EXPECT_STREQ(ex.what(), "test 42\n");
        return;
    }
    FAIL() << "FATAL_ERROR did not throw";
}

TEST(UtilException, BugCheckEvaluation) {
    int checks = 0;
    int arguments = 0;
    BUG_CHECK(++checks == 1, "test %1%", ++arguments);
    EXPECT_EQ(checks, 1);
    EXPECT_EQ(arguments, 0);

    // Conditions can require a contextual conversion to bool.
    auto pointer = std::make_unique<int>(42);
    BUG_CHECK(pointer, "test %1%", ++arguments);
    EXPECT_EQ(arguments, 0);

    const auto line = __LINE__ + 2;
    try {
        BUG_CHECK(++checks == 1, "test %1%", ++arguments);
    } catch (const CompilerBug &ex) {
        EXPECT_EQ(checks, 2);
        EXPECT_EQ(arguments, 1);
        EXPECT_EQ(ex.what(),
                  absl::StrCat("In file: ", __FILE__, ":", line, "\n", cerr_colorize(ANSI_RED),
                               "Compiler Bug", cerr_clear_colors(), ": test 1\n"));
        return;
    }
    FAIL() << "BUG_CHECK did not throw";
}

TEST(UtilException, CheckNullCallerLocation) {
    int *pointer = nullptr;
    const auto line = __LINE__ + 2;
    try {
        CHECK_NULL(pointer);
    } catch (const CompilerBug &ex) {
        EXPECT_EQ(ex.what(),
                  absl::StrCat("In file: ", __FILE__, ":", line, "\n", cerr_colorize(ANSI_RED),
                               "Compiler Bug", cerr_clear_colors(), ": ", __FILE__, ":", line,
                               ": Null pointer\n"));
        return;
    }
    FAIL() << "CHECK_NULL did not throw";
}

TEST(UtilException, RuntimeFormat) {
    const std::string format = "test %1%";
    const auto line = __LINE__ + 2;
    try {
        BUG(format.c_str(), 42);
    } catch (const CompilerBug &ex) {
        EXPECT_EQ(ex.what(),
                  absl::StrCat("In file: ", __FILE__, ":", line, "\n", cerr_colorize(ANSI_RED),
                               "Compiler Bug", cerr_clear_colors(), ": test 42\n"));
        return;
    }
    FAIL() << "BUG did not throw";
}

}  // namespace P4::Util
