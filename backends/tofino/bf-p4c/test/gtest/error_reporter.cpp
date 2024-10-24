/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fstream>

#include "bf-p4c/arch/arch.h"
#include "bf-p4c/logging/collect_diagnostic_checks.h"
#include "bf_gtest_helpers.h"
#include "frontends/parsers/parserDriver.h"
#include "gtest/gtest.h"
#include "lib/error.h"

namespace P4::Test {
class ErrorReporterTest : public ::testing::Test {
 protected:
    std::stringstream customStream;
    std::string ROOT_DIR = std::string(__FILE__);

    void SetUp() {
        // ROOT_DIR currently looks like 'ROOT_DIR/bf-p4c/test/gtest/error_reporter.cpp' -> adjust
        const std::string SUFFIX("/p4c/extensions/bf-p4c/test/gtest/error_reporter.cpp");
        ROOT_DIR = ROOT_DIR.substr(0, ROOT_DIR.size() - std::string(SUFFIX).size());
    }

    void TearDown() {}

    std::ostream *DiagnosticTestSetup(BfErrorReporter &reporter, std::string code) {
        auto testCode = TestCode(TestCode::Hdr::CoreP4, code);

        auto *backup_stream = reporter.getOutputStream();
        reporter.setOutputStream(&customStream);

        const std::string FILE_NAME = "/tmp/preprocessor_input.p4";
        {
            std::ofstream fs(FILE_NAME);
            fs << code;
        }

        auto &opts = BFNContext::get().options();
        opts.file = FILE_NAME;
        BFN::collect_diagnostic_checks(reporter, opts);

        return backup_stream;
    }

    IR::Node *GetMockNode(const std::string &code, int line, int column = 1) {
        Util::SourcePosition pos(line, column);
        auto *sources = new Util::InputSources();
        sources->appendText(code.c_str());

        return new IR::StringLiteral(Util::SourceInfo(sources, pos), ""_cs);
    }

    void ReportError(const std::string &code, std::string msg, int line, int column = 1) {
        ::error(ErrorType::ERR_EXPECTED, (msg + "%1%").c_str(), GetMockNode(code, line, column));
    }

    void ReportWarning(const std::string &code, std::string msg, int line, int column = 1) {
        ::warning(ErrorType::WARN_FAILED, (msg + "%1%").c_str(), GetMockNode(code, line, column));
    }
};

TEST_F(ErrorReporterTest, ErrorHelperPlainFormatsCorrectly) {
    boost::format fmt("Str: %1%, Dec: %2%");

    EXPECT_EQ(::error_helper(fmt, "hello", 10).toString(), "Str: hello, Dec: 10\n");
}

TEST_F(ErrorReporterTest, WarningsConformToExpectedFormat) {
    // NOTE: Warnings are formatted exactly the same as errors

    const std::string EXPECTED_WARN_0 =
        R"(TestCode(44): [--Wwarn=unused] warning: 'val_undefined' is unused
    action do_global_action(in bool make_zero, out bool val_undefined) {
                                                        ^^^^^^^^^^^^^
)";

    const std::string EXPECTED_WARN_1 =
        R"(TestCode(46): [--Wwarn=uninitialized_use] warning: tmp may be uninitialized
        tmp = tmp * (make_zero ? 16w0 : 16w1);
              ^^^
)";

    const std::string EXPECTED_WARN_2 =
        R"(TestCode(44): [--Wwarn=uninitialized_out_param] warning: out parameter 'val_undefined' may be uninitialized when 'do_global_action' terminates
    action do_global_action(in bool make_zero, out bool val_undefined) {
                                                        ^^^^^^^^^^^^^
TestCode(44)
    action do_global_action(in bool make_zero, out bool val_undefined) {
           ^^^^^^^^^^^^^^^^
)";

    const std::string EXPECTED_WARN_3 =
        R"([--Wwarn=uninitialized_use] warning: val_undefined may be uninitialized
TestCode(44): [--Wwarn=uninitialized_use] warning: val_undefined may be uninitialized
    action do_global_action(in bool make_zero, out bool val_undefined) {
                                                        ^^^^^^^^^^^^^
)";

    const std::string EXPECTED_WARN_4 =
        R"(warning: No size defined for table 'TABLE_NAME', setting default size to 512
)";

    const std::string EXPECTED_WARNINGS =
        EXPECTED_WARN_0 + EXPECTED_WARN_1 + EXPECTED_WARN_2 + EXPECTED_WARN_3 + EXPECTED_WARN_4;

    // Running frontend on this code should emit EXPECTED_WARN_0, 1, 2, 3 and 4
    auto CODE = R"(
    header ethernet_t {
        bit<48> src_addr;
    }

    struct Headers {
        ethernet_t eth_hdr;
    }

    action do_global_action(in bool make_zero, out bool val_undefined) {
        bit<16> tmp;
        tmp = tmp * (make_zero ? 16w0 : 16w1);
    }
    control ingress(inout Headers h) {
        bool filler_bool = true;
        bool tmp_bool = false;
        action do_action() {
            do_global_action(tmp_bool, tmp_bool);
        }
        table simple_table {
            key = {
                h.eth_hdr.src_addr: exact;
            }
            actions = {
                do_action();
                do_global_action(true, filler_bool);
            }
        }
        apply {
            simple_table.apply();
        }
    }

    control c<H>(inout H h);
    package top<H>(c<H> _c);
    top(ingress()) main;
    )";

    // Compile program so our custom stream is populated with warnings
    // Instantiation of testCode reinstantiates errorReporter instance
    auto testCode = TestCode(TestCode::Hdr::CoreP4, CODE);

    // errorReporter is now stable, redirect to stringstream
    auto backupStream = BaseCompileContext::get().errorReporter().getOutputStream();
    BaseCompileContext::get().errorReporter().setOutputStream(&customStream);
    // Non-standard architecture, default to TNA
    BFN::Architecture::init("TNA"_cs);
    // Compile and emit warnings 1 and 2
    EXPECT_TRUE(testCode.apply_pass(TestCode::Pass::FullFrontend));

    // Should emit EXPECTED_WARN_3
    ::warning("No size defined for table '%s', setting default size to %d", "TABLE_NAME", 512);

    EXPECT_EQ(EXPECTED_WARNINGS, customStream.str());

    // Restore error stream to original
    BaseCompileContext::get().errorReporter().setOutputStream(backupStream);
}

TEST_F(ErrorReporterTest, WarningWithSuffixConformToExpectedFormat) {
    const std::string EXPECTED_WARN_1 =
        R"(TestCode(44): [--Werror=type-error] error: 'return ix + 1'
                return (ix + 1);
                ^^^^^^
  ---- Actual error:
  TestCode(43): Cannot cast implicitly type 'bit<16>' to type 'bool'
              bool f(in bit<16> ix) {
              ^^^^
  ---- Originating from:
  TestCode(44): Source expression 'ix + 1' produces a result of type 'bit<16>' which cannot be assigned to a left-value with type 'bool'
                  return (ix + 1);
                          ^^^^^^
  TestCode(43)
              bool f(in bit<16> ix) {
              ^^^^
TestCode(42): [--Werror=type-error] error: 'cntr'
        Virtual() cntr = {
                  ^^^^
  ---- Actual error:
  TestCode(43): Cannot unify type 'bool' with type 'bit<16>'
              bool f(in bit<16> ix) {
              ^^^^
  ---- Originating from:
  TestCode(43): Method 'f' does not have the expected type 'f'
              bool f(in bit<16> ix) {
                   ^
  TestCode(38)
      abstract bit<16> f(in bit<16> ix);
                       ^
)";

    auto CODE = R"(
    extern Virtual {
    Virtual();
    abstract bit<16> f(in bit<16> ix);
    }

    control c(inout bit<16> p) {
        Virtual() cntr = {
            bool f(in bit<16> ix) {
                return (ix + 1);
            }
        };

        apply {
            p = cntr.f(6);
        }
    }

    control ctr(inout bit<16> x);
    package top(ctr ctrl);
    top(c()) main;
    )";

    // Compile program so our custom stream is populated with warnings
    // Instantiation of testCode reinstantiates errorReporter instance
    auto testCode = TestCode(TestCode::Hdr::CoreP4, CODE);

    // errorReporter is now stable, redirect to stringstream
    auto backupStream = BaseCompileContext::get().errorReporter().getOutputStream();
    BaseCompileContext::get().errorReporter().setOutputStream(&customStream);

    EXPECT_FALSE(testCode.apply_pass(TestCode::Pass::FullFrontend));

    EXPECT_EQ(EXPECTED_WARN_1, customStream.str());

    // Restore error stream to original
    BaseCompileContext::get().errorReporter().setOutputStream(backupStream);
}

TEST_F(ErrorReporterTest, ParserErrorConformsToExpectedFormat) {
    const std::string EXPECTED_WARN =
        R"(file.cpp(0):syntax error, unexpected IDENTIFIER, expecting {
header hdr bug
           ^^^
)";

    // errorReporter is now stable, redirect to stringstream
    auto backupStream = BaseCompileContext::get().errorReporter().getOutputStream();
    BaseCompileContext::get().errorReporter().setOutputStream(&customStream);

    std::istringstream inputCode = std::istringstream("header hdr bug { bit<8> field; }");
    P4::P4ParserDriver::parse(inputCode, "file.cpp", 1);

    EXPECT_EQ(customStream.str(), EXPECTED_WARN);

    // Restore error stream to original
    BaseCompileContext::get().errorReporter().setOutputStream(backupStream);
}

TEST_F(ErrorReporterTest, NoAssertions) {
    AutoCompileContext ctx(new BFNContext());
    auto &reporter = BFNContext::get().errorReporter();
    const std::string CODE = R"(
        control c(inout bit<16> p) {
            action noop() {}
            table test1 {
                actions = {
                    noop;
                }
                default_action = noop;
            }
            apply {
                test1.apply();
            }
        }
    )";

    auto *backup = DiagnosticTestSetup(reporter, CODE);

    EXPECT_EQ(reporter.verify_checks(), BfErrorReporter::CheckResult::NO_TESTS);

    reporter.setOutputStream(backup);
}

TEST_F(ErrorReporterTest, ErrorAssertionSuccess) {
    AutoCompileContext ctx(new BFNContext());
    auto &reporter = BFNContext::get().errorReporter();
    const std::string CODE = R"(
        control c(inout bit<16> p) { // expect error: "Example error message"
            action noop() {}
            table test1 {
                actions = {
                    noop;
                }
                default_action = noop;
            }
            apply {
                test1.apply();
            }
        }
    )";

    auto *backup = DiagnosticTestSetup(reporter, CODE);

    ReportError(CODE, "Example error message", 2);

    EXPECT_EQ(reporter.verify_checks(), BfErrorReporter::CheckResult::SUCCESS);

    reporter.setOutputStream(backup);
}

TEST_F(ErrorReporterTest, ErrorCheckNotMatched) {
    AutoCompileContext ctx(new BFNContext());
    auto &reporter = BFNContext::get().errorReporter();
    const std::string CODE = R"(
        control c(inout bit<16> p) { /* expect error: "Example multi
line \"error\" message" */
            action noop() {}
            table test1 {
                actions = {
                    noop;
                }
                default_action = noop;
            }
            apply {
                test1.apply();
            }
        }
    )";
    const std::string ERROR =
        "[--Werror=not-found] error: Unmatched check: Expected error message \"Example multi\n"
        "line \\\"error\\\" message\" at line 2 not reported.\n";

    auto *backup = DiagnosticTestSetup(reporter, CODE);

    // No errors.

    EXPECT_EQ(reporter.verify_checks(), BfErrorReporter::CheckResult::FAILURE);
    EXPECT_EQ(customStream.str(), ERROR);

    reporter.setOutputStream(backup);
}

TEST_F(ErrorReporterTest, UnexpectedError) {
    AutoCompileContext ctx(new BFNContext());
    auto &reporter = BFNContext::get().errorReporter();
    const std::string CODE = R"(
        control c(inout bit<16> p) { /* expect error@+5: "Example multi
line error message" */
            action noop() {}
            table test1 {
                actions = {
                    noop;
                }
                default_action = noop;
            }
            apply {
                test1.apply();
            }
        }
    )";
    const std::string ERROR =
        "[--Werror=unexpected] error: The following errors were not expected:\n"
        "(5): [--Werror=expected] error: Unexpected error!";

    auto *backup = DiagnosticTestSetup(reporter, CODE);

    ReportError(CODE, "Example multi\nline error message", 7);
    ReportError(CODE, "Unexpected error!", 5);

    EXPECT_EQ(reporter.verify_checks(), BfErrorReporter::CheckResult::FAILURE);
    EXPECT_NE(customStream.str().find(ERROR), std::string::npos);

    reporter.setOutputStream(backup);
}

TEST_F(ErrorReporterTest, ErrorAndWarningSameLine) {
    AutoCompileContext ctx(new BFNContext());
    auto &reporter = BFNContext::get().errorReporter();
    const std::string CODE = R"(
        control c(inout bit<16> p) { // expect error: "Error message"
        // expect warning@-1: "Warning message"
            action noop() {}
            table test1 {
                actions = {
                    noop;
                }
                default_action = noop;
            }
            apply {
                test1.apply();
            }
        }
    )";
    auto *backup = DiagnosticTestSetup(reporter, CODE);

    // Once an error has been reported, warnings are silenced, so the warning has to be reported
    // first.
    ReportWarning(CODE, "Warning message", 2);
    ReportError(CODE, "Error message", 2);

    EXPECT_EQ(reporter.verify_checks(), BfErrorReporter::CheckResult::SUCCESS);

    reporter.setOutputStream(backup);
}

}  // namespace P4::Test
