// SPDX-FileCopyrightText: 2026 The P4 Language Consortium & Devansh Singh
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "frontends/common/parseInput.h"
#include "frontends/p4/toP4/toP4.h"
#include "helpers.h"
#include "ir/ir.h"
#include "lib/sourceCodeBuilder.h"

namespace P4::Test {

namespace {

/// Parses @p source in a fresh compile context and prints it back with ToP4.
std::optional<std::string> reprint(const std::string &source) {
    AutoCompileContext autoContext(std::make_unique<GTestContext>());
    const auto *program = parseP4String(source);
    if (program == nullptr || ::P4::errorCount() > 0) return std::nullopt;
    Util::SourceCodeBuilder builder;
    ToP4 top4(builder, false);
    program->apply(top4);
    return std::string(builder.toString());
}

std::string printLiteral(const IR::StringLiteral *literal) {
    Util::SourceCodeBuilder builder;
    ToP4 top4(builder, false);
    literal->apply(top4);
    return std::string(builder.toString());
}

}  // namespace

struct ToP4Test : P4CTest {};

TEST_F(ToP4Test, StringLiteralsSurviveReprinting) {
    // The lexer keeps escape sequences and newlines of a string literal as written.
    const std::vector<std::string> literals = {
        R"("")",          R"("plain")",
        R"("a\"b")",      R"("a\\b")",
        R"("a\\\"b")",    R"("a\nb")",
        R"("caf\u00e9")", "\"line1\nline2\"",
        "\"tab\there\"",  "\"line1\n    line2\n\"",
        R"("\\")",
    };
    for (const auto &literal : literals) {
        const std::string source = "extern E { E(string s); }\nE(" + literal + ") e;\n";
        const auto first = reprint(source);
        ASSERT_TRUE(first) << literal;
        EXPECT_NE(std::string::npos, first->find("E(" + literal + ")"))
            << "The literal is not printed as written:\n"
            << *first;
        const auto second = reprint(*first);
        ASSERT_TRUE(second) << literal;
        EXPECT_EQ(*first, *second) << literal;
    }
}

TEST_F(ToP4Test, QuotesThatTheLexerCannotProduceAreEscaped) {
    // Unescaped quotes from IR transforms must be escaped so output stays parseable.
    EXPECT_EQ(R"("a\"b")", printLiteral(IR::StringLiteral::get("a\"b"_cs)));
    // Already-escaped quotes and backslashes escaping other characters stay untouched
    EXPECT_EQ(R"("a\"b")", printLiteral(IR::StringLiteral::get(R"(a\"b)"_cs)));
    EXPECT_EQ(R"("a\\")", printLiteral(IR::StringLiteral::get(R"(a\\)"_cs)));
    // An even number of backslashes leaves the trailing quote unescaped.
    EXPECT_EQ(R"("a\\\"b")", printLiteral(IR::StringLiteral::get(R"(a\\"b)"_cs)));
}

}  // namespace P4::Test
