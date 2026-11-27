// SPDX-FileCopyrightText: 2026 The P4 Language Consortium
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

/// Parses @p source and prints it back, or returns std::nullopt if parsing fails.
std::optional<std::string> reprint(const std::string &source) {
    // Diagnostics accumulate in the context, so every program gets a fresh one.
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
    // Values created by the compiler may contain a quote that is not escaped; printing it as is
    // would end the literal early.
    EXPECT_EQ(R"("a\"b")", printLiteral(IR::StringLiteral::get("a\"b"_cs)));
    // Already escaped quotes are left alone, and so are backslashes that escape something else.
    EXPECT_EQ(R"("a\"b")", printLiteral(IR::StringLiteral::get(R"(a\"b)"_cs)));
    EXPECT_EQ(R"("a\\")", printLiteral(IR::StringLiteral::get(R"(a\\)"_cs)));
    // An even number of backslashes does not escape the quote that follows.
    EXPECT_EQ(R"("a\\\"b")", printLiteral(IR::StringLiteral::get(R"(a\\"b)"_cs)));
}

}  // namespace P4::Test
