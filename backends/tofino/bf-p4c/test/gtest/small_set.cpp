/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#include <unordered_set>

#include "bf-p4c/lib/small_set.h"
#include "gtest/gtest.h"

namespace P4::Test {

TEST(SmallSet, Move) {
    const std::size_t elems = 32;
    SmallSet<int, elems> set1({2, 1, 3, 7});
    auto copy = set1;

    auto moved = std::move(set1);
    ASSERT_EQ(copy, moved);

    moved.reserve(elems);
    for (int i = 0; i < 64; ++i) {
        moved.insert(i);
    }
    copy = moved;
    auto moved_again = std::move(moved);
    ASSERT_EQ(copy, moved_again);
    ASSERT_TRUE(moved.empty());
}

TEST(SmallSet, Insert) {
    SmallSet<int, 2ul> set1;
    set1.insert(3);
    set1.insert(5);
    auto [it1, inserted1] = set1.insert(8);
    ASSERT_TRUE(inserted1);
    ASSERT_EQ(*it1, 8);

    auto [it2, inserted2] = set1.insert(5);
    ASSERT_FALSE(inserted2);
    ASSERT_EQ(*it2, 5);

    ASSERT_EQ(set1.size(), 3ul);
    ASSERT_TRUE(set1.count(3));
    ASSERT_TRUE(set1.count(5));
    ASSERT_TRUE(set1.count(8));
    ASSERT_TRUE(set1.contains(3));
    ASSERT_TRUE(set1.contains(5));
    ASSERT_TRUE(set1.contains(8));

    SmallSet<int, 2ul> set2({3});
    set2.insert(set1.begin(), set1.end());
    ASSERT_EQ(set1, set2);
}

TEST(SmallSet, Erase) {
    SmallSet<int> set1({0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
    auto erased = set1.erase(5);
    ASSERT_FALSE(set1.count(5));
    ASSERT_FALSE(set1.contains(5));
    ASSERT_EQ(erased, 1ul);

    erased = set1.erase(9);
    ASSERT_FALSE(set1.count(9));
    ASSERT_FALSE(set1.contains(9));
    ASSERT_EQ(erased, 1ul);

    auto set2 = set1;
    SmallSet<int> to_remove1({8, 4, 2});
    set1.erase_set(to_remove1);
    ASSERT_EQ(set1.size(), 5ul);
    ASSERT_FALSE(set1.count(8));
    ASSERT_FALSE(set1.count(4));
    ASSERT_FALSE(set1.count(2));
    ASSERT_FALSE(set1.contains(8));
    ASSERT_FALSE(set1.contains(4));
    ASSERT_FALSE(set1.contains(2));

    std::unordered_set<int> to_remove2(to_remove1.begin(), to_remove1.end());
    set2.erase_set(to_remove2);
    ASSERT_EQ(set1, set2);
}

}  // namespace P4::Test
