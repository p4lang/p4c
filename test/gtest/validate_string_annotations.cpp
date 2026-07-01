// SPDX-FileCopyrightText: 2026 The P4 Language Consortium & Devansh Singh <devansh.jay.singh@gmail.com>

//
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include <optional>
#include <string>

#include "absl/strings/substitute.h"
#include "lib/error.h"
#include "test/gtest/helpers.h"

namespace P4::Test {

namespace {

std::optional<FrontendTestCase> createTestCase(const std::string &annotatedDecl) {
    auto source = P4_SOURCE(P4Headers::NONE, R"(
$0
    )");
    return FrontendTestCase::create(absl::Substitute(source, annotatedDecl));
}

}  // namespace

class ValidateStringAnnotations : public P4CTest {};

// A plain, valid @name annotation should compile without any errors.
TEST_F(ValidateStringAnnotations, ValidNameAnnotation) {
    RedirectStderr errors;
    auto test = createTestCase(R"(@name("foo") action bar() { })");
    errors.dumpAndReset();
    ASSERT_TRUE(test);
    EXPECT_EQ(::P4::errorCount(), 0);
}

// Same as above, but for @deprecated and @noWarn.
TEST_F(ValidateStringAnnotations, ValidDeprecatedAndNoWarnAnnotations) {
    RedirectStderr errors;
    auto test = createTestCase(R"(
        @deprecated("do not use") action bar() { }
        @noWarn("unused") action baz() { }
    )");
    errors.dumpAndReset();
    ASSERT_TRUE(test);
    EXPECT_EQ(::P4::errorCount(), 0);
}

// A non-string argument must be rejected with a clean diagnostic.
TEST_F(ValidateStringAnnotations, NonStringArgumentIsRejected) {
    RedirectStderr errors;
    auto test = createTestCase(R"(@name(5) action bar() { })");
    errors.dumpAndReset();
    EXPECT_FALSE(test);
    EXPECT_EQ(::P4::errorCount(), 1);
    EXPECT_TRUE(errors.contains("value must be a string"));
}

// An annotation with no arguments fails to parse as a single expression, so it
// remains unparsed; ValidateStringAnnotations must not crash trying to call
// getExpr() on it, and the earlier parse error must still be reported.
TEST_F(ValidateStringAnnotations, EmptyArgumentListDoesNotCrash) {
    RedirectStderr errors;
    auto test = createTestCase(R"(@name() action bar() { })");
    errors.dumpAndReset();
    EXPECT_FALSE(test);
    EXPECT_GT(::P4::errorCount(), 0);
}

// An annotation with more than one argument also fails to parse as a single
// expression; this must not crash and must still be reported as an error.
TEST_F(ValidateStringAnnotations, TooManyArgumentsDoesNotCrash) {
    RedirectStderr errors;
    auto test = createTestCase(R"(@name("a", "b") action bar() { })");
    errors.dumpAndReset();
    EXPECT_FALSE(test);
    EXPECT_GT(::P4::errorCount(), 0);
}

// Regression test: a structured key-value annotation (@name[key=value]) does
// not carry an expression list at all. Before the fix, calling getExpr() on
// it triggered an internal BUG (a compiler crash) instead of a clean
// diagnostic. This must now be rejected gracefully.
TEST_F(ValidateStringAnnotations, StructuredKeyValueAnnotationDoesNotCrash) {
    RedirectStderr errors;
    auto test = createTestCase(R"(@name[key="foo"] action bar() { })");
    errors.dumpAndReset();
    EXPECT_FALSE(test);
    EXPECT_EQ(::P4::errorCount(), 1);
    EXPECT_TRUE(errors.contains("structured annotation syntax"));
}

// A structured expression-list annotation (@name[...]) is also not a valid
// way to write these built-in annotations, even though it happens to carry an
// expression list and would not have crashed before the fix.
TEST_F(ValidateStringAnnotations, StructuredExpressionListAnnotationIsRejected) {
    RedirectStderr errors;
    auto test = createTestCase(R"(@name["foo"] action bar() { })");
    errors.dumpAndReset();
    EXPECT_FALSE(test);
    EXPECT_EQ(::P4::errorCount(), 1);
    EXPECT_TRUE(errors.contains("structured annotation syntax"));
}

// Same structured key-value regression, but for @deprecated.
TEST_F(ValidateStringAnnotations, StructuredKeyValueDeprecatedDoesNotCrash) {
    RedirectStderr errors;
    auto test = createTestCase(R"(@deprecated[key="foo"] action bar() { })");
    errors.dumpAndReset();
    EXPECT_FALSE(test);
    EXPECT_EQ(::P4::errorCount(), 1);
}

// Same structured key-value regression, but for @noWarn.
TEST_F(ValidateStringAnnotations, StructuredKeyValueNoWarnDoesNotCrash) {
    RedirectStderr errors;
    auto test = createTestCase(R"(@noWarn[key="foo"] action baz() { })");
    errors.dumpAndReset();
    EXPECT_FALSE(test);
    EXPECT_EQ(::P4::errorCount(), 1);
}

}  // namespace P4::Test
