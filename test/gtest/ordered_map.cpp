// SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
// Copyright 2013-present Barefoot Networks, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "lib/ordered_map.h"

#include <gtest/gtest.h>

#include "lib/map.h"

namespace P4::Test {

TEST(OrderedMap, MapEqual) {
    ordered_map<unsigned, unsigned> a;
    ordered_map<unsigned, unsigned> b;

    EXPECT_TRUE(a == b);

    a[1] = 111;
    a[2] = 222;
    a[3] = 333;
    a[4] = 444;

    b[1] = 111;
    b[2] = 222;
    b[3] = 333;
    b[4] = 444;

    EXPECT_TRUE(a == b);

    a.erase(2);
    b.erase(2);

    EXPECT_TRUE(a == b);

    a.clear();
    b.clear();

    EXPECT_TRUE(a == b);
}

TEST(OrderedMap, MapNotEqual) {
    ordered_map<unsigned, unsigned> a;
    ordered_map<unsigned, unsigned> b;

    EXPECT_TRUE(a == b);

    a[1] = 111;
    a[2] = 222;
    a[3] = 333;
    a[4] = 444;

    b[4] = 444;
    b[3] = 333;
    b[2] = 222;
    b[1] = 111;

    EXPECT_TRUE(a != b);

    a.clear();
    b.clear();

    EXPECT_TRUE(a == b);

    a[1] = 111;
    a[2] = 222;

    b[1] = 111;
    b[2] = 222;
    b[3] = 333;

    EXPECT_TRUE(a != b);

    a.clear();
    b.clear();

    EXPECT_TRUE(a == b);

    a[1] = 111;
    a[2] = 222;
    a[3] = 333;
    a[4] = 444;

    b[4] = 111;
    b[3] = 222;
    b[2] = 333;
    b[1] = 444;

    EXPECT_TRUE(a != b);

    a.clear();
    b.clear();

    EXPECT_TRUE(a == b);

    a[1] = 111;
    a[2] = 222;
    a[3] = 333;
    a[4] = 444;

    b[1] = 111;
    b[2] = 111;
    b[3] = 111;
    b[4] = 111;

    EXPECT_TRUE(a != b);
}

TEST(OrderedMap, InsertEmplaceErase) {
    ordered_map<unsigned, unsigned> om;
    std::map<unsigned, unsigned> sm;

    auto it = om.end();
    for (auto v : {0, 1, 2, 3, 4, 5, 6, 7, 8}) {
        sm.emplace(v, 2 * v);
        std::pair<unsigned, unsigned> pair{v, 2 * v};
        if (v % 2 == 0) {
            if ((v / 2) % 2 == 0) {
                it = om.insert(pair).first;
            } else {
                it = om.emplace(v, pair.second).first;
            }
        } else {
            if ((v / 2) % 2 == 0) {
                it = om.insert(std::move(pair)).first;
            } else {
                it = om.emplace(v, v * 2).first;
            }
        }
    }

    EXPECT_TRUE(std::equal(om.begin(), om.end(), sm.begin(), sm.end()));

    it = std::next(om.begin(), 2);
    om.erase(it);
    sm.erase(std::next(sm.begin(), 2));

    EXPECT_TRUE(om.size() == sm.size());
    EXPECT_TRUE(std::equal(om.begin(), om.end(), sm.begin(), sm.end()));
}

TEST(OrderedMap, ExistingKey) {
    ordered_map<int, std::string> myMap{{1, "One"}, {2, "Two"}, {3, "Three"}};

    EXPECT_EQ(get(myMap, 1), "One");
    EXPECT_EQ(get(myMap, 2), "Two");
    EXPECT_EQ(get(myMap, 3), "Three");
}

TEST(OrderedMap, NonExistingKey) {
    ordered_map<int, std::string> myMap{{1, "One"}, {2, "Two"}, {3, "Three"}};

    EXPECT_EQ(get(myMap, 4), "");
}

}  // namespace P4::Test
