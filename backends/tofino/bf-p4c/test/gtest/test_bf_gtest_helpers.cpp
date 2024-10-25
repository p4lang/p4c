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

#include <boost/algorithm/string/replace.hpp>

#include "bf-p4c/midend/elim_cast.h"
#include "bf_gtest_helpers.h"
#include "frontends/p4/toP4/toP4.h"
#include "gtest/gtest.h"
#include "lib/sourceCodeBuilder.h"

namespace P4::Test {

using namespace Match;  // To remove noise & reduce line lengths.

TEST(testBfGtestHelper, MatchTrimWhiteSpace) {
    EXPECT_EQ(trimWhiteSpace("").compare(""), 0);
    EXPECT_EQ(trimWhiteSpace("a b").compare("a b"), 0);
    EXPECT_EQ(trimWhiteSpace(" \n a \n b \n ").compare("a b"), 0);
}

TEST(testBfGtestHelper, MatchTrimAnnotations) {
    EXPECT_EQ(trimAnnotations("").compare(""), 0);
    EXPECT_EQ(trimAnnotations(" a @word b ").compare(" a  b "), 0);
    EXPECT_EQ(trimAnnotations(" a @func() b ").compare(" a  b "), 0);
    EXPECT_EQ(trimAnnotations(" a @func(\"string\") b ").compare(" a  b "), 0);
    EXPECT_EQ(trimAnnotations(" a @word b @func() c ").compare(" a  b  c "), 0);
    EXPECT_EQ(trimAnnotations(" a @func(p1,\np2\n) b ").compare(" a  b "), 0);
    EXPECT_EQ(trimAnnotations(" a @func(p1,\n@name\n) b ").compare(" a  b "), 0);

    // ********* We do not handle nested parentheses **********
    EXPECT_THROW(trimAnnotations(" a @nest(() b "), std::invalid_argument);
    EXPECT_THROW(trimAnnotations(" a @func( fx() ) b "), std::invalid_argument);
}

TEST(testBfGtestHelper, MatchConvetToRegex) {
    EXPECT_EQ(convert_to_regex("").compare(""), 0);
    EXPECT_EQ(convert_to_regex("a b").compare("a b"), 0);
    EXPECT_EQ(convert_to_regex("a `b` c").compare("a b c"), 0);
    EXPECT_EQ(convert_to_regex("^$.|?*+(){[\\").compare(R"(\^\$\.\|\?\*\+\(\)\{\[\\)"), 0);
    EXPECT_EQ(convert_to_regex("`^$.|?*+(){[\\`").compare("^$.|?*+(){[\\"), 0);  // Don't convert.
}

TEST(testBfGtestHelper, MatchMatchBasic) {
    EXPECT_EQ(match_basic("", ""), 0U);
    EXPECT_EQ(match_basic("", "expression"), 0U);
    EXPECT_EQ(match_basic("expression", "expression"), 10U);
    EXPECT_EQ(match_basic("exp", "expression"), 3U);
    EXPECT_EQ(match_basic("ress", "expression", 3), 3U + 4U);
    EXPECT_EQ(match_basic("ress", "expression", 3, 3 + 4), 3U + 4U);
    EXPECT_EQ(match_basic("ress", "expression", 3, 99), 3U + 4U);
    EXPECT_EQ(match_basic("ress", "expression", 3, std::string::npos), 3U + 4U);
    EXPECT_EQ(match_basic("a \n b", "a \n bcd"), 5U);

    EXPECT_EQ(match_basic("expression", ""), failed);
    EXPECT_EQ(match_basic("ress", "expre", 3), failed);
    EXPECT_EQ(match_basic("ress", "expression"), failed);
    EXPECT_EQ(match_basic("ress", "expression", 3, 4), failed);
    EXPECT_EQ(match_basic("ress", "expression", 3, 1), failed);  // Bad pos.
    EXPECT_EQ(match_basic("ress", "expression", 13), failed);    // Bad pos.
}

TEST(testBfGtestHelper, MatchResult) {
    EXPECT_TRUE(Result(true, 1, 2) == Result(true, 1, 2));
    EXPECT_TRUE(Result(true, 1, 2) != Result(false, 1, 2));
    EXPECT_TRUE(Result(true, 1, 2) != Result(true, 2, 2));
    EXPECT_TRUE(Result(true, 1, 2) != Result(true, 1, 1));
}

TEST(testBfGtestHelper, MatchMatch) {
    // Same as match_basic()
    EXPECT_EQ(match(CheckList{""}, ""), Result(true, 0, 1));
    EXPECT_EQ(match(CheckList{""}, "expression"), Result(true, 0, 1));
    EXPECT_EQ(match(CheckList{"expression"}, "expression"), Result(true, 10, 1));
    EXPECT_EQ(match(CheckList{"exp"}, "expression"), Result(true, 3, 1));
    EXPECT_EQ(match(CheckList{"ress"}, "expression", 3), Result(true, 3 + 4, 1));
    EXPECT_EQ(match(CheckList{"ress"}, "expression", 3, 3 + 4), Result(true, 3 + 4, 1));
    EXPECT_EQ(match(CheckList{"ress"}, "expression", 3, 99), Result(true, 3 + 4, 1));
    EXPECT_EQ(match(CheckList{"ress"}, "expression", 3, std::string::npos), Result(true, 3 + 4, 1));
    EXPECT_EQ(match(CheckList{"a \n b"}, "a \n bcd"), Result(true, 5, 1));

    EXPECT_FALSE(match(CheckList{"expression"}, "").success);
    EXPECT_EQ(match(CheckList{"expression"}, ""), Result(false, 0, 0));
    EXPECT_EQ(match(CheckList{"ress"}, "expre", 3), Result(false, 3, 0));
    EXPECT_EQ(match(CheckList{"ress"}, "expression"), Result(false, 0, 0));
    EXPECT_EQ(match(CheckList{"ress"}, "expression", 3, 4), Result(false, 3, 0));
    EXPECT_EQ(match(CheckList{"ress"}, "expression", 3, 1), Result(false, failed, 0));  // Bad pos.
    EXPECT_EQ(match(CheckList{"ress"}, "expression", 13), Result(false, failed, 0));    // Bad pos.

    // Inline checking.
    EXPECT_TRUE(match(CheckList{""}, "").success);
    EXPECT_FALSE(match(CheckList{"expression"}, "").success);

    // And with regex.
    EXPECT_EQ(match(CheckList{"``"}, ""), Result(true, 0, 1));
    EXPECT_EQ(match(CheckList{"``"}, " "), Result(true, 0, 1));
    EXPECT_EQ(match(CheckList{"` `"}, " "), Result(true, 1, 1));
    EXPECT_EQ(match(CheckList{"` `"}, ""), Result(false, 0, 0));
    EXPECT_THROW(match(CheckList{"`"}, "stuff"), std::invalid_argument);
    EXPECT_THROW(match(CheckList{"` ` `"}, "stuff"), std::invalid_argument);

    EXPECT_EQ(match(CheckList{"\\x60"}, "`"), Result(true, 1, 1));
    EXPECT_EQ(match(CheckList{"`\\x60`"}, "`"), Result(true, 1, 1));

    EXPECT_EQ(match(CheckList{"The `(\\w+)` is `(\\d+)`"}, "The num is 42!"), Result(true, 13, 1));
    EXPECT_EQ(match(CheckList{"How `(\\w+)` you `\\1`"}, "How do you do???"), Result(true, 13, 1));
    EXPECT_EQ(match(CheckList{"How `(\\w+)` you `(\\1)`"}, "How do you do?"), Result(true, 13, 1));
    EXPECT_ANY_THROW(match(CheckList{"`(\\w+)` `\\2`"}, "Bad Bad back-reference!"));
    EXPECT_EQ(match(CheckList{"One `(\\w+)` 3"}, "One Two 3 Four"), Result(true, 9, 1));
    EXPECT_EQ(match(CheckList{"One `(\\w+)` 3"}, "One One Two 3 Four"), Result(false, 0, 0));

    // And with CheckList.
    EXPECT_EQ(match(CheckList{"hello", "world"}, "hello world!"), Result(false, 5, 1));
    EXPECT_EQ(
        match(CheckList{"hello", "world"}, "hello world!", 0, std::string::npos, TrimWhiteSpace),
        Result(true, 11, 2));  // No space.
    EXPECT_EQ(
        match(CheckList{"hello ", "world"}, "hello world!", 0, std::string::npos, TrimWhiteSpace),
        Result(true, 11, 2));  // Left space.
    EXPECT_EQ(
        match(CheckList{"hello", " world"}, "hello world!", 0, std::string::npos, TrimWhiteSpace),
        Result(true, 11, 2));  // Right space.
    EXPECT_EQ(
        match(CheckList{"hello ", " world"}, "hello world!", 0, std::string::npos, TrimWhiteSpace),
        Result(false, 6, 1));  // Two spaces!
    EXPECT_EQ(match(CheckList{"The", "`(\\w+)`", "is", "`(\\d+)`"}, "The num is 42!", 0,
                    std::string::npos, TrimWhiteSpace),
              Result(true, 13, 4));
    // Back References across CheckList are fine.
    EXPECT_EQ(match(CheckList{"How", "`(\\w+)`", "you", "`\\1`"}, "How do you do???", 0,
                    std::string::npos, TrimWhiteSpace),
              Result(true, 13, 4));
    EXPECT_EQ(match(CheckList{"`(\\w+)`", "`(\\w\\w)`", "cad", "`\\1`", "`\\2`"}, "abracadabra!", 0,
                    std::string::npos, TrimWhiteSpace),
              Result(true, 11, 5));

    // Check for early miss-match & use 'pos' to skip over first word.
    auto res = match(CheckList{"One", "`(\\w+)`", "3"}, "One One Two 3 Four", 0, std::string::npos,
                     TrimWhiteSpace);
    EXPECT_FALSE(res.success) << " pos=" << res.pos << " count=" << res.count << "'\n";
    res = match(CheckList{"One", "`(\\w+)`", "3"}, "One One Two 3 Four", 3, std::string::npos,
                TrimWhiteSpace);
    EXPECT_TRUE(res.success) << " pos=" << res.pos << " count=" << res.count << "'\n";
}

TEST(testBfGtestHelper, MatchFindNextEnd) {
    EXPECT_THROW(find_next_end("", 0, ""), std::invalid_argument);     // Bad ends string
    EXPECT_THROW(find_next_end("", 0, "1"), std::invalid_argument);    // Bad ends string
    EXPECT_THROW(find_next_end("", 0, "123"), std::invalid_argument);  // Bad ends string
    EXPECT_THROW(find_next_end("", 0, "11"), std::invalid_argument);   // Bad ends string
    EXPECT_EQ(find_next_end("", 0, "12"), std::string::npos);

    EXPECT_EQ(find_next_end("   ", 0, "{}"), std::string::npos);
    EXPECT_EQ(find_next_end("0{2", 0, "{}"), std::string::npos);
    EXPECT_EQ(find_next_end("0{2{45}7}9", 0, "{}"), std::string::npos);
    EXPECT_EQ(find_next_end("{12}4}6", 0, "{}"), 5U);
    EXPECT_EQ(find_next_end("{12}4}6", 2, "{}"), 3U);
    EXPECT_EQ(find_next_end("AAZZZAZ", 0, "AZ"), 4U);
}

TEST(testBfGtestHelper, MatchFindNextBlock) {
    EXPECT_THROW(find_next_block("", 0, ""), std::invalid_argument);  // Bad ends string
    EXPECT_EQ(find_next_block("   ", 0, "{}"),
              std::make_pair(std::string::npos, std::string::npos));
    EXPECT_EQ(find_next_block("0{2", 0, "{}"), std::make_pair(1UL, std::string::npos));
    EXPECT_EQ(find_next_block("0{2{45}7}9", 0, "{}"), std::make_pair(1UL, 8UL));
    EXPECT_EQ(find_next_block("{12}4}6", 0, "{}"), std::make_pair(0UL, 3UL));
    EXPECT_EQ(find_next_block("{12}4}6", 2, "{}"),
              std::make_pair(std::string::npos, std::string::npos));
    EXPECT_EQ(find_next_block("AAZZZAZ", 0, "AZ"), std::make_pair(0UL, 3UL));
}

TEST(testBfGtestHelper, MatchGetEnds) {
    EXPECT_EQ(get_ends('*'), "");
    EXPECT_EQ(get_ends('{'), "{}");
    EXPECT_EQ(get_ends('}'), "");
    EXPECT_EQ(get_ends('('), "()");
    EXPECT_EQ(get_ends(')'), "");
    EXPECT_EQ(get_ends('['), "[]");
    EXPECT_EQ(get_ends(']'), "");
    EXPECT_EQ(get_ends('<'), "<>");
    EXPECT_EQ(get_ends('>'), "");
    auto str = "{12}4}6";
    EXPECT_EQ(find_next_block(str, 0, get_ends(str[0])), std::make_pair(0UL, 3UL));
}

namespace {
const char *Code = R"(
    control TestIngress<H, M>(inout H hdr, inout M meta);
    package TestPackage<H, M>(TestIngress<H, M> ig);
    %0%
    control testingress(inout Headers headers, inout Metadata meta) {
        %1%
    }
    TestPackage(testingress()) main;)";
const char *EmptyDefs = R"(struct Metadata{}; struct Headers{};)";
const char *EmptyAppy = R"(apply{})";
const char *Marker = "control testingress`([^\\{]*\\{)`";
}  // namespace

TEST(testBfGtestHelper, TestCodeTestCodeGood) {
    TestCode(TestCode::Hdr::None, "");
    TestCode(TestCode::Hdr::Tofino1arch, "");
    TestCode(TestCode::Hdr::Tofino2arch, "");
    auto blk2018 = TestCode(TestCode::Hdr::V1model_2018, "");
    auto blk2020 = TestCode(TestCode::Hdr::V1model_2020, "");
    EXPECT_NE(blk2018.extract_code().compare(blk2020.extract_code()), 0);

    // The 'Code' string requires two insertions.
    TestCode(TestCode::Hdr::Tofino1arch, Code, {EmptyDefs, EmptyAppy});
    TestCode(TestCode::Hdr::Tofino1arch, Code, {EmptyDefs, EmptyAppy}, Marker);
    TestCode(TestCode::Hdr::Tofino1arch, Code, {EmptyDefs, EmptyAppy}, Marker, {});
}

TEST(testBfGtestHelperDeathTest, TestCodeTestCodeOptions) {
    // N.B. The first argument is always 'TestCode<N>' followed by the 'options' arguments.
    ::testing::FLAGS_gtest_death_test_style = "threadsafe";
    EXPECT_EXIT(
        TestCode(TestCode::Hdr::Tofino1arch, Code, {EmptyDefs, EmptyAppy}, Marker, {"--version"}),
        ::testing::ExitedWithCode(0), "TestCode[^V]*Version");
}

TEST(testBfGtestHelper, TestCodeTestCodeBad) {
    testing::internal::CaptureStderr();
    EXPECT_THROW(TestCode(TestCode::Hdr::Tofino1arch, Code, {EmptyDefs}), std::invalid_argument);
    auto stderr = testing::internal::GetCapturedStderr();
    EXPECT_NE(stderr.find("syntax error"), std::string::npos);

    testing::internal::CaptureStderr();
    EXPECT_THROW(TestCode(TestCode::Hdr::Tofino1arch, Code, {EmptyDefs, ""}),
                 std::invalid_argument);
    stderr = testing::internal::GetCapturedStderr();
    EXPECT_NE(stderr.find("syntax error"), std::string::npos);

    testing::internal::CaptureStderr();
    EXPECT_THROW(TestCode(TestCode::Hdr::Tofino1arch, Code, {"", EmptyAppy}),
                 std::invalid_argument);
    stderr = testing::internal::GetCapturedStderr();
    EXPECT_NE(stderr.find("syntax error"), std::string::npos);

    testing::internal::CaptureStderr();
    EXPECT_THROW(TestCode(TestCode::Hdr::Tofino1arch, Code, {EmptyDefs, EmptyAppy}, "bad"),
                 std::invalid_argument);
    stderr = testing::internal::GetCapturedStderr();
    EXPECT_EQ(stderr.length(), 0U);

    TestCode(TestCode::Hdr::Tofino1arch, Code, {EmptyDefs, EmptyAppy, "There is no %3%"});
    // Multiple insertions tested below.
}

TEST(testBfGtestHelper, TestCodeTestControlBlock) {
    TestCode(TestCode::TestControlBlock(EmptyDefs, EmptyAppy));
}

TEST(testBfGtestHelper, TestCodeGetBlock) {
    auto blk =
        TestCode(TestCode::Hdr::Tofino1arch, Code, {EmptyDefs, "\napply\n\n{\n\n}\n"}, Marker);
    // The default setting is  blk.flags(TrimWhiteSpace);
    EXPECT_EQ(blk.extract_code().compare("apply { }"), 0);
    EXPECT_EQ(blk.extract_code(6).compare("{ }"), 0);

    blk =
        TestCode(TestCode::Hdr::Tofino1arch, Code, {EmptyDefs, "\n\napply\n\n{\n\n}\n\n"}, Marker);
    blk.flags(Raw);
    // This 'Raw' test is dependant upon the parser, hence is brittle.
    // EXPECT_EQ(blk.extract_code().compare("   apply {\n    }"), 0);

    blk =
        TestCode(TestCode::Hdr::Tofino1arch, Code, {EmptyDefs, "action a(){} apply{a();}"}, Marker);
    blk.flags(TrimWhiteSpace | TrimAnnotations);
    EXPECT_EQ(blk.extract_code().compare("action a() { } apply { a(); }"), 0);

    std::string pkg = Code;
    boost::replace_all(pkg, R"(%1%)", R"(action a(){} %2% apply {%tmp% %tmp%})");
    boost::replace_all(pkg, R"(%0%)", R"(%1%)");
    boost::replace_all(pkg, R"(%tmp%)", R"(%0%)");
    // N.B. control block starts at %2%, also the order of defines is not important.
    blk = TestCode(TestCode::Hdr::Tofino1arch, pkg, {R"(a();)", EmptyDefs, ""}, Marker);
    blk.flags(TrimWhiteSpace | TrimAnnotations);
    EXPECT_EQ(blk.extract_code().compare("action a() { } apply { a(); a(); }"), 0) << blk;
}

TEST(testBfGtestHelper, TestCodeApplyPassConst) {
    auto blk = TestCode(TestCode::Hdr::Tofino1arch, Code, {EmptyDefs, EmptyAppy});
    Util::SourceCodeBuilder builder;
    EXPECT_TRUE(blk.apply_pass(new P4::ToP4(builder, false)));
}

#if 1
TEST(testBfGtestHelper, TestCodeApplyPassMutating) {
    // A similar test is run in the gtest 'ElimCast.EquLeft'.
    // It is here as an example & for debugging purposes.
    auto defs = R"(
        header H { bit<8> field1; bit<8> field2;}
        struct headers_t { H h; }
        struct local_metadata_t {} )";
    auto input = R"(
        action act() {}
        apply {
            if (hdr.h.field1 ++ hdr.h.field2 == 16w0x0102) {
                act();
            }
        })";
    CheckList expected{
        "action act() { }", "apply {", "if (hdr.h.field1 == 8w1 && hdr.h.field2 == 8w2) {",
        "act();",           "}",       "}"};
    auto blk = TestCode(TestCode::Hdr::Tofino1arch, TestCode::tofino_shell(),
                        {defs, TestCode::empty_state(), input, TestCode::empty_appy()},
                        TestCode::tofino_shell_control_marker());
    blk.flags(TrimWhiteSpace | TrimAnnotations);
    EXPECT_TRUE(blk.apply_pass(TestCode::Pass::FullFrontend));
    EXPECT_TRUE(blk.apply_pass(new BFN::RewriteConcatToSlices()));
    auto res = blk.match(expected);
    EXPECT_TRUE(res.success) << "    @ expected[" << res.count << "], char pos=" << res.pos << "\n"
                             << "    '" << expected[res.count] << "'\n"
                             << "    '" << blk.extract_code(res.pos) << "'\n";
}
#endif

TEST(testBfGtestHelper, TestCodeApplyPreconstructedPasses) {
    std::string EmptyDefs = "struct local_metadata_t{}; struct headers_t{};";
    auto blk = TestCode(
        TestCode::Hdr::TofinoMin, TestCode::tofino_shell(),
        {EmptyDefs, TestCode::empty_state(), TestCode::empty_appy(), TestCode::empty_appy()});
    EXPECT_TRUE(blk.CreateBlockThreadLocalInstances());
    // If applying FullBackend, ThreadLocalInstances pass will run twice. And "thread-name::" will
    // be prepended to the names of headers and metadata structs twice. So disable apply FullBackend
    // for now.
    // EXPECT_TRUE(blk.apply_pass(TestCode::Pass::FullBackend));
}

TEST(testBfGtestHelper, TestP4CodeMatch) {
    auto blk = TestCode(TestCode::Hdr::Tofino1arch, Code, {EmptyDefs, EmptyAppy}, Marker);
    auto res = blk.match(CheckList{"apply", "{", "}"});
    EXPECT_TRUE(res.success) << " pos=" << res.pos << " count=" << res.count << "\n    '"
                             << blk.extract_code() << "'\n";
    ;
    // EXPECT_EQ(res.pos, 9U);   // Dependant upon parser.
    EXPECT_EQ(res.count, 3U);

    blk = TestCode(TestCode::Hdr::Tofino1arch, Code, {EmptyDefs, EmptyAppy}, Marker);
    blk.flags(Raw);
    res = blk.match(CheckList{"`\\s*`apply`\\s*`{`\\s*`}`\\s*`"});
    EXPECT_TRUE(res.success) << " pos=" << res.pos << " count=" << res.count << "\n    '"
                             << blk.extract_code() << "'\n";
    // EXPECT_EQ(res.pos, 16U);   // Dependant upon parser.
    EXPECT_EQ(res.count, 1U);
}

TEST(testBfGtestHelper, TestAsmCodeMatch) {
    std::string EmptyDefs = "struct local_metadata_t{}; struct headers_t{};";
    auto blk = TestCode(
        TestCode::Hdr::TofinoMin, TestCode::tofino_shell(),
        {EmptyDefs, TestCode::empty_state(), TestCode::empty_appy(), TestCode::empty_appy()},
        TestCode::tofino_shell_control_marker());
    blk.flags(TrimWhiteSpace | TrimAnnotations);
    EXPECT_TRUE(blk.CreateBackend());
    EXPECT_TRUE(blk.apply_pass(TestCode::Pass::FullBackend));

    EXPECT_EQ(blk.extract_code(), blk.extract_code(TestCode::CodeBlock::P4Code));

    auto res = blk.match(TestCode::CodeBlock::P4Code, CheckList{"apply", "{", "}"});
    EXPECT_TRUE(res.success) << " pos=" << res.pos << " count=" << res.count << "\n'"
                             << blk.extract_code(TestCode::CodeBlock::P4Code) << "'\n";

    res = blk.match(TestCode::CodeBlock::PhvAsm, CheckList{"phv ingress:", "`.*`", "phv egress:"});
    EXPECT_TRUE(res.success) << " pos=" << res.pos << " count=" << res.count << "\n'"
                             << blk.extract_code(TestCode::CodeBlock::PhvAsm) << "'\n";

    res = blk.match(TestCode::CodeBlock::MauAsm, CheckList{"stage 0 ingress:"});
    EXPECT_TRUE(res.success) << " pos=" << res.pos << " count=" << res.count << "\n'"
                             << blk.extract_code(TestCode::CodeBlock::MauAsm) << "'\n";

    res = blk.match(TestCode::CodeBlock::ParserIAsm, CheckList{"parser ingress: start"});
    EXPECT_TRUE(res.success) << " pos=" << res.pos << " count=" << res.count << "\n'"
                             << blk.extract_code(TestCode::CodeBlock::ParserIAsm) << "'\n";

    res = blk.match(TestCode::CodeBlock::DeparserIAsm, CheckList{"deparser ingress:"});
    EXPECT_TRUE(res.success) << " pos=" << res.pos << " count=" << res.count << "\n'"
                             << blk.extract_code(TestCode::CodeBlock::DeparserIAsm) << "'\n";

    res = blk.match(TestCode::CodeBlock::ParserEAsm, CheckList{"parser egress: start"});
    EXPECT_TRUE(res.success) << " pos=" << res.pos << " count=" << res.count << "\n'"
                             << blk.extract_code(TestCode::CodeBlock::ParserEAsm) << "'\n";

    res = blk.match(TestCode::CodeBlock::DeparserEAsm, CheckList{"deparser egress:"});
    EXPECT_TRUE(res.success) << " pos=" << res.pos << " count=" << res.count << "\n'"
                             << blk.extract_code(TestCode::CodeBlock::DeparserEAsm) << "'\n";
}

}  // namespace P4::Test
