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

#include "gtest/gtest.h"
#include "bf-p4c/lib/union_find.hpp"
#include "lib/ordered_set.h"

namespace P4::Test {

namespace {

using namespace P4;

/** @returns true if @left and @right contain the same elements. */
template <typename T>
static bool equivalent(const ordered_set<T>& left, const ordered_set<T>& right) {
    if (left.size() != right.size())
        return false;
    for (auto x : left)
        if (left.find(x) == right.end())
            return false;
    return true;
}

}   // namespace

TEST(UnionFind, ops) {
    ordered_set<int> universe({ 1, 2, 3 });
    UnionFind<int> uf(universe);

    // After creation, all elements are in singleton sets.
    EXPECT_TRUE(equivalent(ordered_set<int>({1}), uf.setOf(uf.find(1))));
    EXPECT_TRUE(equivalent(ordered_set<int>({2}), uf.setOf(uf.find(2))));
    EXPECT_TRUE(equivalent(ordered_set<int>({3}), uf.setOf(uf.find(3))));

    // Union should merge exactly the sets of 1 and 2.
    uf.makeUnion(1, 2);

    EXPECT_TRUE(equivalent(ordered_set<int>({1, 2}), uf.setOf(uf.find(1))));
    EXPECT_TRUE(equivalent(ordered_set<int>({1, 2}), uf.setOf(uf.find(2))));
    EXPECT_TRUE(equivalent(ordered_set<int>({3}), uf.setOf(uf.find(3))));

    // Union should be idempotent.
    uf.makeUnion(1, 2);

    EXPECT_TRUE(equivalent(ordered_set<int>({1, 2}), uf.setOf(uf.find(1))));
    EXPECT_TRUE(equivalent(ordered_set<int>({1, 2}), uf.setOf(uf.find(2))));
    EXPECT_TRUE(equivalent(ordered_set<int>({3}), uf.setOf(uf.find(3))));

    uf.makeUnion(3, 2);

    EXPECT_TRUE(equivalent(ordered_set<int>({1, 2, 3}), uf.setOf(uf.find(1))));
    EXPECT_TRUE(equivalent(ordered_set<int>({1, 2, 3}), uf.setOf(uf.find(2))));
    EXPECT_TRUE(equivalent(ordered_set<int>({1, 2, 3}), uf.setOf(uf.find(3))));
}

TEST(UnionFind, copy) {
    UnionFind uf({1, 2, 3, 4});
    UnionFind uf2(uf);
    UnionFind<int> uf3;

    uf.makeUnion(1, 2);
    uf2.makeUnion(4, 2);

    EXPECT_TRUE(equivalent(ordered_set{4}, uf.setOf(4)));
    EXPECT_TRUE(equivalent(ordered_set{1, 2}, uf.setOf(2)));
    EXPECT_TRUE(equivalent(ordered_set{4, 2}, uf2.setOf(4)));
    EXPECT_TRUE(equivalent(ordered_set{4, 2}, uf2.setOf(2)));

    EXPECT_TRUE(equivalent(ordered_set{1, 2}, uf.setOf(1)));
    EXPECT_TRUE(equivalent(ordered_set{1, 2}, uf.setOf(2)));
    EXPECT_TRUE(equivalent(ordered_set{1}, uf2.setOf(1)));
    EXPECT_TRUE(equivalent(ordered_set{2, 4}, uf2.setOf(2)));

    EXPECT_TRUE(equivalent(ordered_set{3}, uf.setOf(3)));
    EXPECT_TRUE(equivalent(ordered_set{3}, uf2.setOf(3)));

    uf.insert(5);
    EXPECT_TRUE(equivalent(ordered_set{5}, uf.setOf(5)));
    for (auto set : uf2) {
        EXPECT_FALSE(set.count(5));
    }

    uf3 = uf2;  // copy assignment operator, not constuctor
    uf3.makeUnion(4, 3);

    EXPECT_TRUE(equivalent(ordered_set{4, 2}, uf2.setOf(4)));
    EXPECT_TRUE(equivalent(ordered_set{4, 2}, uf2.setOf(2)));
    EXPECT_TRUE(equivalent(ordered_set{3}, uf2.setOf(3)));
    EXPECT_TRUE(equivalent(ordered_set{1}, uf2.setOf(1)));

    EXPECT_TRUE(equivalent(ordered_set{4, 2, 3}, uf3.setOf(4)));
    EXPECT_TRUE(equivalent(ordered_set{4, 2, 3}, uf3.setOf(2)));
    EXPECT_TRUE(equivalent(ordered_set{4, 2, 3}, uf3.setOf(3)));
    EXPECT_TRUE(equivalent(ordered_set{1}, uf3.setOf(1)));
}

TEST(UnionFind, move) {
    UnionFind uf{1, 2, 3, 4};

    uf.makeUnion(1, 2);

    UnionFind uf2(std::move(uf));
    UnionFind<int> uf3;

    EXPECT_EQ(uf.numSets(), 0u);

    EXPECT_TRUE(equivalent(ordered_set{1, 2}, uf2.setOf(1)));
    EXPECT_TRUE(equivalent(ordered_set{1, 2}, uf2.setOf(2)));
    EXPECT_TRUE(equivalent(ordered_set{3}, uf2.setOf(3)));
    EXPECT_TRUE(equivalent(ordered_set{4}, uf2.setOf(4)));

    uf2.makeUnion(4, 2);

    uf3 = std::move(uf2);

    EXPECT_EQ(uf2.numSets(), 0u);

    EXPECT_TRUE(equivalent(ordered_set{1, 2, 4}, uf3.setOf(1)));
    EXPECT_TRUE(equivalent(ordered_set{1, 2, 4}, uf3.setOf(2)));
    EXPECT_TRUE(equivalent(ordered_set{3}, uf3.setOf(3)));
    EXPECT_TRUE(equivalent(ordered_set{1, 2, 4}, uf3.setOf(4)));
}

}   /* end namespace P4::Test */
