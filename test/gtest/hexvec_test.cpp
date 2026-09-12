// SPDX-FileCopyrightText: 2026 The P4 Language Consortium & Devansh Singh
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include <sstream>
#include <vector>

#include "lib/hex.h"

namespace P4::Test {

namespace {

// Format a vector into its string hex-vector representation.
template <typename T>
std::string format(std::vector<T> &v) {
    std::ostringstream out;
    out << hexvec(v);
    return out.str();
}

}  // namespace

TEST(HexVec, Empty) {
    std::vector<int> v;
    EXPECT_EQ(format(v), "[]");
}

TEST(HexVec, SingleElement) {
    std::vector<uint8_t> v = {0xab};
    EXPECT_EQ(format(v), "[ab]");
}

TEST(HexVec, MultipleElements) {
    std::vector<uint8_t> v = {0x01, 0xff, 0x0a};
    EXPECT_EQ(format(v), "[1 ff a]");
}

}  // namespace P4::Test
