// Copyright 2024 Intel Corp.
// SPDX-FileCopyrightText: 2024 Intel Corp.
//
// SPDX-License-Identifier: Apache-2.0

#include "lib/map.h"

#include <gtest/gtest.h>

#include <algorithm>

namespace P4::Test {

TEST(ValuesForKey, set_equal) {
    std::multimap<int, int> a;
    std::multimap<int, int> b;

    // Populate a
    a.emplace(1, 1);
    a.emplace(1, 2);
    a.emplace(1, 3);
    a.emplace(1, 4);

    a.emplace(2, 1);
    a.emplace(2, 2);

    a.emplace(3, 1);

    EXPECT_FALSE(a == b);

    // Walk through the elements in a and populate in b
    std::set<int> keys({1, 2, 3});
    for (auto k : keys) {
        for (auto v : ValuesForKey(a, k)) {
            b.emplace(k, v);
        }
    }

    EXPECT_TRUE(a == b);
}

}  // namespace P4::Test
