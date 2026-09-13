// SPDX-FileCopyrightText: 2026 The P4 Language Consortium & Devansh Singh
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include "frontends/common/parser_options.h"
#include "lib/error_reporter.h"
#include "test/gtest/helpers.h"

namespace P4::Test {

using namespace P4::literals;

namespace {

// Process a single option argument and return the active ErrorReporter instance.
ErrorReporter &processOption(const char *option) {
    auto &options = GTestContext::get().options();
    const char *argv[] = {"test", option};
    options.process(2, const_cast<char *const *>(argv));
    return GTestContext::get().errorReporter();
}

// Retrieve the diagnostic action for a specific option name.
DiagnosticAction actionFor(ErrorReporter &reporter, cstring name) {
    return reporter.getDiagnosticAction(ErrorType::WARN_INVALID, name, DiagnosticAction::Warn);
}

}  // namespace

class DiagnosticOptions : public P4CTest {};

TEST_F(DiagnosticOptions, WdisableSingleDiagnostic) {
    auto &reporter = processOption("--Wdisable=some-warning");
    EXPECT_EQ(DiagnosticAction::Ignore, actionFor(reporter, "some-warning"_cs));
}

TEST_F(DiagnosticOptions, WdisableCommaSeparatedDiagnostics) {
    auto &reporter = processOption("--Wdisable=first-warning,second-warning");
    EXPECT_EQ(DiagnosticAction::Ignore, actionFor(reporter, "first-warning"_cs));
    EXPECT_EQ(DiagnosticAction::Ignore, actionFor(reporter, "second-warning"_cs));
}

TEST_F(DiagnosticOptions, WerrorCommaSeparatedDiagnostics) {
    auto &reporter = processOption("--Werror=first-warning,second-warning");
    EXPECT_EQ(DiagnosticAction::Error, actionFor(reporter, "first-warning"_cs));
    EXPECT_EQ(DiagnosticAction::Error, actionFor(reporter, "second-warning"_cs));
}

TEST_F(DiagnosticOptions, WinfoCommaSeparatedDiagnostics) {
    auto &reporter = processOption("--Winfo=first-warning,second-warning");
    EXPECT_EQ(DiagnosticAction::Info, actionFor(reporter, "first-warning"_cs));
    EXPECT_EQ(DiagnosticAction::Info, actionFor(reporter, "second-warning"_cs));
}

TEST_F(DiagnosticOptions, WwarnCommaSeparatedDiagnostics) {
    auto &reporter = processOption("--Wwarn=first-error,second-error");
    EXPECT_EQ(DiagnosticAction::Warn, actionFor(reporter, "first-error"_cs));
    EXPECT_EQ(DiagnosticAction::Warn, actionFor(reporter, "second-error"_cs));
}

}  // namespace P4::Test
