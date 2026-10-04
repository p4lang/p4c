/*
 * SPDX-FileCopyrightText: 2026 The P4 Language Consortium
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <utility>

#include "backends/p4fmt/formatTokens.h"
#include "backends/p4fmt/syntaxRecovery.h"

namespace P4::P4Fmt {
namespace {

TEST(P4FmtSource, LosslessPartition) {
    const FormatTokens source("// header\r\nbit<8> x = 8w0xff; /* body */ \"//\\\"#\"\n");
    ASSERT_TRUE(source.complete);
    std::string reconstructed;
    size_t end = 0;
    for (const auto &token : source.tokens) {
        EXPECT_EQ(end, token.begin);
        EXPECT_GT(token.end, token.begin);
        reconstructed += source.spelling(token);
        end = token.end;
    }
    EXPECT_EQ(source.source, reconstructed);
    EXPECT_FALSE(SyntaxRecovery::preserveWholeFile(source));
}

TEST(P4FmtSource, PreservesCommentsAtEveryBoundary) {
    const FormatTokens source("// header\nx=/* value */1; // tail\n/* eof */");
    EXPECT_EQ(SyntaxRecovery::reconcile(source, "x = 1;\n"),
              "// header\nx =/* value */1; // tail\n/* eof */");
    EXPECT_EQ(SyntaxRecovery::reconcile(FormatTokens("{/* empty */}"), "{\n}\n"),
              "{/* empty */}\n");
    EXPECT_EQ(SyntaxRecovery::reconcile(FormatTokens("// only"), ""), "// only");
}

TEST(P4FmtSource, RejectsChangesToSpellingAndTokenBoundaries) {
    for (const auto &[before, after] : {
             std::pair{"x=0x01;", "x = 1;"},
             std::pair{"x=(1);", "x = 1;"},
             std::pair{"a + +b", "a ++ b"},
             std::pair{"a > > b", "a >> b"},
             std::pair{"a / * b", "a /* b */"},
             std::pair{"a b", "ab"},
             std::pair{"@ pragma(1)", "@pragma(1)"},
             std::pair{"x;", "x; y;"},
             std::pair{"x; y;", "x;"},
         }) {
        EXPECT_FALSE(SyntaxRecovery::reconcile(FormatTokens(before), after)) << before;
    }
}

TEST(P4FmtSource, PreservesUnsupportedInput) {
    for (auto text : {"#include <missing.p4>\n", "#vendor extension\n", "x\\\ny",
                      "// comment\\\r\ncontinued", "/* unterminated", "\"unterminated"}) {
        EXPECT_TRUE(SyntaxRecovery::preserveWholeFile(FormatTokens(text))) << text;
    }
    EXPECT_FALSE(SyntaxRecovery::preserveWholeFile(FormatTokens("{#}")));
    EXPECT_FALSE(SyntaxRecovery::preserveWholeFile(FormatTokens("\"#define X\" /* # */")));
}

TEST(P4FmtSource, SpliceBeforeCommentRecognition) {
    const std::string text = "/\\\n/ comment\\\r\ncontinued\nfoo\\\nbar +\\\n + value";
    const FormatTokens source(text);
    ASSERT_TRUE(source.complete);
    std::string reconstructed;
    std::vector<std::string_view> logical;
    for (const auto &token : source.tokens) {
        reconstructed += source.spelling(token);
        if (token.kind != FormatTokens::Kind::Whitespace)
            logical.push_back(source.logicalSpelling(token));
    }
    EXPECT_EQ(reconstructed, text);
    ASSERT_EQ(logical.size(), 5u);
    EXPECT_EQ(logical[0], "// commentcontinued");
    EXPECT_EQ(logical[1], "foobar");
    EXPECT_EQ(logical[2], "+");
    EXPECT_EQ(logical[3], "+");
}

TEST(P4FmtSource, EmptyInput) {
    EXPECT_EQ(SyntaxRecovery::reconcile(FormatTokens(""), ""), "");
    EXPECT_TRUE(FormatTokens("").tokens.empty());
}

TEST(P4FmtSource, CleansPrinterWhitespace) {
    EXPECT_EQ(SyntaxRecovery::reconcile(FormatTokens("x; y;"), "x; \t\n    y;\n\n"),
              "x;\n    y;\n");
    EXPECT_EQ(SyntaxRecovery::reconcile(FormatTokens("x; // keep  \n"), "x;\n\n"),
              "x; // keep  \n");
}

}  // namespace
}  // namespace P4::P4Fmt
