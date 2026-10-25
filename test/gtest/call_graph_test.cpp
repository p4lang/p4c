// SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
// Copyright 2013-present Barefoot Networks, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include <vector>

#include "frontends/p4/callGraph.h"

namespace P4::Test {

template <class T>
static void sameSet(std::unordered_set<T> &set, std::vector<T> vector) {
    EXPECT_EQ(vector.size(), set.size());
    for (T v : vector) EXPECT_NEQ(set.end(), set.find(v));
}

template <class T>
static void sameSet(std::set<T> &set, std::vector<T> vector) {
    EXPECT_EQ(vector.size(), set.size());
    for (T v : vector) EXPECT_NEQ(set.end(), set.find(v));
}

TEST(CallGraph, Acyclic) {
    P4::CallGraph<char> acyclic("acyclic");
    // a->b->c
    // \_____^

    acyclic.calls('a', 'b');
    acyclic.calls('a', 'c');
    acyclic.calls('b', 'c');

    std::vector<char> sorted;
    acyclic.sort(sorted);
    EXPECT_EQ(3u, sorted.size());
    EXPECT_EQ('c', sorted.at(0));
    EXPECT_EQ('b', sorted.at(1));
    EXPECT_EQ('a', sorted.at(2));
}

}  // namespace P4::Test
