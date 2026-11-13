// SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
// Copyright 2013-present Barefoot Networks, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "lib/source_file.h"

#include <gtest/gtest.h>

#include <string>
#include <string_view>

#include "lib/compile_context.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"
#include "lib/json.h"

namespace P4::Util {

TEST(UtilSourceFile, SourcePosition) {
    SourcePosition invalid;
    EXPECT_FALSE(invalid.isValid());

    SourcePosition position(3, 3);
    EXPECT_EQ("3:3", position.toString());

    auto &context = BaseCompileContext::get();
    cstring str = context.errorReporter().format_message("%1% - %2%", position, position);
    EXPECT_EQ("3:3 - 3:3\n", str);

    SourcePosition next(3, 4);
    EXPECT_LT(position, next);
    EXPECT_LE(position, position);
    EXPECT_EQ(position, position);
}

TEST(UtilSourceFile, InputSources) {
    Util::InputSources sources;
    sources.appendToLastLine("First line");
    {
        SourcePosition position = sources.getCurrentPosition();
        EXPECT_EQ(1u, position.getLineNumber());
        EXPECT_EQ(10u, position.getColumnNumber());
    }

    sources.appendNewline("\n");
    sources.mapLine("fakesource.p4", 5);

    {
        SourcePosition position = sources.getCurrentPosition();
        EXPECT_EQ(2u, position.getLineNumber());
        EXPECT_EQ(0u, position.getColumnNumber());
    }

    sources.appendToLastLine("Second line");
    sources.appendNewline("\n");
    sources.appendToLastLine("Third line");
    sources.appendNewline("\n");
    sources.seal();

    EXPECT_EQ(3u, sources.lineCount());

    auto fl = sources.getLine(1);
    EXPECT_EQ("First line\n", fl);

    auto sl = sources.getLine(2);
    EXPECT_EQ("Second line\n", sl);

    SourceFileLine original = sources.getSourceLine(3);
    EXPECT_EQ("fakesource.p4", original.fileName);
    EXPECT_EQ(5u, original.sourceLine);
}

TEST(UtilSourceFile, SourceInfo) {
    Util::InputSources sources;

    SourcePosition t1_s(1, 1);
    SourcePosition t1_e(1, 5);

    SourceInfo t1 = SourceInfo(&sources, t1_s, t1_e);

    SourcePosition t2_s(1, 8);
    SourcePosition t2_e(2, 2);

    SourceInfo t2 = SourceInfo(&sources, t2_s, t2_e);

    SourceInfo span = t1 + t2;
    cstring str = span.toString();
    EXPECT_EQ("(1:1)-(2:2)", str);

    SourceInfo invalid;
    EXPECT_FALSE(invalid.isValid());
}

namespace {

/// Returns the brief source fragment for the first occurrence of @p needle in the one-line
/// source @p text.
cstring briefFragmentOf(const char *text, std::string_view needle) {
    InputSources sources;
    sources.appendText(text);
    sources.seal();
    const auto column = std::string_view(text).find(needle);
    SourceInfo info(&sources, SourcePosition(1, column), SourcePosition(1, column + needle.size()));
    return info.toBriefSourceFragment();
}

}  // namespace

TEST(UtilSourceFile, BriefSourceFragment) {
    EXPECT_EQ("hdr.a", briefFragmentOf("x = hdr.a;\n", "hdr.a"));
    EXPECT_EQ("a = 's';", briefFragmentOf("a = 's';", "a = 's';"));
}

TEST(UtilSourceFile, BriefSourceFragmentOnLineWithQuotes) {
    // Only the requested range is returned, not the whole line.
    EXPECT_EQ("hdr.a", briefFragmentOf("log_msg(\"hello\"); x = hdr.a;\n", "hdr.a"));
}

TEST(UtilSourceFile, BriefSourceFragmentKeepsQuotes) {
    // The fragment is raw source text; JSON escaping is the serializer's job.
    EXPECT_EQ("@name(\"ingress.t\")",
              briefFragmentOf("@name(\"ingress.t\") table t {}\n", "@name(\"ingress.t\")"));
}

TEST(UtilSourceFile, BriefSourceFragmentWithoutTrailingNewline) {
    // No character may be dropped from a quoted line that has no line terminator.
    EXPECT_EQ("a = \"s\";", briefFragmentOf("a = \"s\";", "a = \"s\";"));
}

TEST(UtilSourceFile, BriefSourceFragmentMultiLineSpan) {
    // A span that continues on later lines is cut at the end of its first line and marked,
    // whether or not that line contains quotes.
    for (const auto *text :
         {"if (x == 'a') {\n  y = 1;\n}\n", "if (x == \"a\") {\n  y = 1;\n}\n"}) {
        InputSources sources;
        sources.appendText(text);
        sources.seal();
        SourceInfo info(&sources, SourcePosition(1, 0), SourcePosition(3, 1));
        const std::string expected = std::string(text).substr(0, 15) + " ...";
        EXPECT_EQ(expected, info.toBriefSourceFragment()) << text;
    }
}

TEST(UtilSourceFile, BriefSourceFragmentEscapedOnceInJson) {
    const auto fragment = briefFragmentOf("a = \"s\";\n", "\"s\"");
    ASSERT_EQ("\"s\"", fragment);
    EXPECT_EQ("\"\\\"s\\\"\"", JsonValue(fragment).toString());
}

}  // namespace P4::Util
