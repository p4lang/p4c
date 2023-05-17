/*
Copyright 2013-present Barefoot Networks, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "lib/ordered_set.h"

#include <algorithm>

#include "gtest/gtest.h"

namespace Test {

TEST(ordered_set, set_equal) {
    ordered_set<unsigned> a;
    ordered_set<unsigned> b;

    EXPECT_TRUE(a == b);

    a.insert(1);
    a.insert(2);
    a.insert(3);
    a.insert(4);

    b.insert(1);
    b.insert(2);
    b.insert(3);
    b.insert(4);

    EXPECT_TRUE(a == b);

    a.erase(2);
    b.erase(2);

    EXPECT_TRUE(a == b);

    a.clear();
    b.clear();

    EXPECT_TRUE(a == b);
}

TEST(ordered_set, set_not_equal) {
    ordered_set<unsigned> a;
    ordered_set<unsigned> b;

    EXPECT_TRUE(a == b);

    a.insert(1);
    a.insert(2);
    a.insert(3);
    a.insert(4);

    b.insert(4);
    b.insert(3);
    b.insert(2);
    b.insert(1);

    EXPECT_TRUE(a != b);

    a.clear();
    b.clear();

    EXPECT_TRUE(a == b);

    a.insert(1);
    a.insert(2);

    b.insert(1);
    b.insert(2);
    b.insert(3);

    EXPECT_TRUE(a != b);
}

TEST(ordered_set, insert_erase_iterators) {
    ordered_set<unsigned> set;
    std::vector<unsigned> vec;

    typename ordered_set<unsigned>::const_iterator it = set.end();
    for (auto v : {0, 1, 2, 3, 4}) {
        vec.push_back(v);
        if (v % 2 == 0) {
            it = std::next(set.insert(it, v));
        } else {
            it = std::next(set.insert(it, std::move(v)));
        }
    }

    EXPECT_TRUE(std::equal(set.begin(), set.end(), vec.begin(), vec.end()));

    it = std::next(set.begin(), 2);
    set.erase(it);
    vec.erase(vec.begin() + 2);

    EXPECT_TRUE(set.size() == vec.size());
    EXPECT_TRUE(std::equal(set.begin(), set.end(), vec.begin(), vec.end()));
}

TEST(ordered_set, set_intersect) {
    ordered_set<unsigned> a = {5, 8, 1, 10, 4};
    ordered_set<unsigned> b = {4, 2, 9, 5, 1};
    ordered_set<unsigned> expect = {1, 4, 5};
    ordered_set<unsigned> res;
    std::set_intersection(a.sorted_begin(), a.sorted_end(), b.sorted_begin(), b.sorted_end(),
                          std::inserter(res, res.end()));

    EXPECT_EQ(res, expect);
}

TEST(ordered_set, x_is_strict_prefix_of_y) {
    ordered_set<unsigned> x;
    x.insert(1);

    ordered_set<unsigned> y;
    y.insert(1);
    EXPECT_FALSE(x < y);
    EXPECT_FALSE(y < x);

    y.insert(2);
    EXPECT_TRUE(x < y);
    EXPECT_FALSE(y < x);
}

}  // namespace Test
