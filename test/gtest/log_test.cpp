// SPDX-FileCopyrightText: 2026 The P4 Language Consortium & Devansh Singh
// SPDX-License-Identifier: Apache-2.0

#include "lib/log.h"

#include <gtest/gtest.h>

#include <set>
#include <sstream>
#include <vector>

namespace P4::Test {

namespace {
template <typename T>
std::string render(const T &container) {
    std::ostringstream out;
    out << container;
    return out.str();
}
}  // namespace

TEST(FormatContainer, EmptyContainerHasNoInnerPadding) {
    EXPECT_EQ("[]", render(std::vector<int>{}));
    EXPECT_EQ("()", render(std::set<int>{}));
}

TEST(FormatContainer, NonEmptyContainerHasNoOuterPadding) {
    // Spaces separate elements from each other, but not from the
    // delimiters: "[1, 2, 3]", not "[ 1, 2, 3 ]".
    EXPECT_EQ("[1, 2, 3]", render(std::vector<int>{1, 2, 3}));
    EXPECT_EQ("[42]", render(std::vector<int>{42}));
    EXPECT_EQ("(5, 6, 7)", render(std::set<int>{5, 6, 7}));
}

}  // namespace P4::Test