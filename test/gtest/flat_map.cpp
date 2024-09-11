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

#include "lib/flat_map.h"

#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include <stdexcept>

#include "lib/map.h"

using ::testing::ElementsAre;
using ::testing::ElementsAreArray;

namespace P4::Test {

TEST(FlatMap, RangeConstructor) {
    flat_map<unsigned, unsigned>::value_type myMap[] = {{1, 1}, {1, 2}, {1, 3}, {2, 1}, {2, 2},
                                                        {2, 3}, {3, 1}, {3, 2}, {3, 3}};

    flat_map<unsigned, unsigned> first(std::begin(myMap), std::end(myMap));
    EXPECT_THAT(first,
                ElementsAre(std::make_pair(1, 1), std::make_pair(2, 1), std::make_pair(3, 1)));
}

TEST(FlatMap, InitializerListConstructor) {
    flat_map<unsigned, unsigned> myMap(
        {{1, 1}, {2, 2}, {3, 3}, {4, 4}, {5, 5}, {1, 2}, {10, 10}, {8, 8}});
    EXPECT_THAT(myMap, ElementsAre(std::make_pair(1, 1), std::make_pair(2, 2), std::make_pair(3, 3),
                                   std::make_pair(4, 4), std::make_pair(5, 5), std::make_pair(8, 8),
                                   std::make_pair(10, 10)));
}

TEST(FlatMap, InitializerListAssignment) {
    flat_map<unsigned, unsigned> myMap;
    myMap = {{1, 1}, {2, 2}};
    EXPECT_THAT(myMap, ElementsAre(std::make_pair(1, 1), std::make_pair(2, 2)));
}

TEST(FlatMap, InsertFindSize) {
    flat_map<unsigned, unsigned> s;
    s.insert(std::make_pair(1, 1));
    s.insert(std::make_pair(1, 1));
    s.insert(std::make_pair(2, 2));

    EXPECT_EQ(2u, s.size());
    EXPECT_EQ(std::make_pair(1u, 1u), *s.find(1));
    EXPECT_EQ(std::make_pair(2u, 2u), *s.find(2));
    EXPECT_EQ(s.end(), s.find(7));
}

TEST(FlatMap, CopySwap) {
    flat_map<unsigned, unsigned> original;
    original.insert({1, 1});
    original.insert({2, 2});
    EXPECT_THAT(original, ElementsAre(std::make_pair(1, 1), std::make_pair(2, 2)));

    flat_map<unsigned, unsigned> copy(original);
    EXPECT_THAT(copy, ElementsAre(std::make_pair(1, 1), std::make_pair(2, 2)));

    copy.erase(copy.begin());
    copy.insert({10, 10});
    EXPECT_THAT(copy, ElementsAre(std::make_pair(2, 2), std::make_pair(10, 10)));

    original.swap(copy);
    EXPECT_THAT(original, ElementsAre(std::make_pair(2, 2), std::make_pair(10, 10)));
    EXPECT_THAT(copy, ElementsAre(std::make_pair(1, 1), std::make_pair(2, 2)));
}

// operator[](const Key&)
TEST(FlatMap, SubscriptConstKey) {
    flat_map<std::string, unsigned> m;

    // Default construct elements that don't exist yet.
    unsigned &s = m["a"];
    EXPECT_EQ(0, s);
    EXPECT_EQ(1u, m.size());

    // The returned mapped reference should refer unsignedo the map.
    s = 22;
    EXPECT_EQ(22, m["a"]);

    // Overwrite existing elements.
    m["a"] = 44;
    EXPECT_EQ(44, m["a"]);
}

// mapped_type& at(const Key&)
// const mapped_type& at(const Key&) const
TEST(FlatMap, At) {
    flat_map<unsigned, std::string> m = {{1, "a"}, {2, "b"}};

    // basic usage.
    EXPECT_EQ("a", m.at(1));
    EXPECT_EQ("b", m.at(2));

    // const reference works.
    const std::string &const_ref = std::as_const(m).at(1);
    EXPECT_EQ("a", const_ref);

    // reference works, can operate on the string.
    m.at(1)[0] = 'x';
    EXPECT_EQ("x", m.at(1));

    // out-of-bounds will throw.
    EXPECT_THROW(m.at(-1), std::out_of_range);
    EXPECT_THROW({ m.at(-1)[0] = 'z'; }, std::out_of_range);

    // heterogeneous look-up works.
    flat_map<std::string, unsigned> m2 = {{"a", 1}, {"b", 2}};
    EXPECT_EQ(1, m2.at(std::string_view("a")));
    EXPECT_EQ(2, std::as_const(m2).at(std::string_view("b")));
}

TEST(FlatMap, MapEqual) {
    flat_map<unsigned, unsigned> a;
    flat_map<unsigned, unsigned> b;

    EXPECT_EQ(a, b);

    a[1] = 111;
    a[2] = 222;
    a[3] = 333;
    a[4] = 444;

    b[1] = 111;
    b[2] = 222;
    b[3] = 333;
    b[4] = 444;

    EXPECT_EQ(a, b);

    a.erase(2);
    b.erase(2);

    EXPECT_EQ(a, b);

    a.clear();
    b.clear();

    EXPECT_EQ(a, b);
}

TEST(FlatMap, MapNotEqual) {
    flat_map<unsigned, unsigned> a;
    flat_map<unsigned, unsigned> b;

    EXPECT_EQ(a, b);

    a[1] = 111;
    a[2] = 222;
    a[3] = 333;
    a[4] = 444;

    b[4] = 444;
    b[3] = 333;
    b[2] = 222;
    b[1] = 111;

    EXPECT_EQ(a, b);

    a.clear();
    b.clear();

    EXPECT_EQ(a, b);

    a[1] = 111;
    a[2] = 222;

    b[1] = 111;
    b[2] = 222;
    b[3] = 333;

    EXPECT_NE(a, b);

    a.clear();
    b.clear();

    EXPECT_EQ(a, b);

    a[1] = 111;
    a[2] = 222;
    a[3] = 333;
    a[4] = 444;

    b[4] = 111;
    b[3] = 222;
    b[2] = 333;
    b[1] = 444;

    EXPECT_NE(a, b);

    a.clear();
    b.clear();

    EXPECT_EQ(a, b);

    a[1] = 111;
    a[2] = 222;
    a[3] = 333;
    a[4] = 444;

    b[1] = 111;
    b[2] = 111;
    b[3] = 111;
    b[4] = 111;

    EXPECT_NE(a, b);
}

TEST(FlatMap, InsertEmplaceErase) {
    flat_map<unsigned, unsigned> om;
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

    EXPECT_THAT(om, ElementsAreArray(sm.begin(), sm.end()));

    it = std::next(om.begin(), 2);
    om.erase(it);
    sm.erase(std::next(sm.begin(), 2));

    EXPECT_EQ(om.size(), sm.size());

    EXPECT_THAT(om, ElementsAreArray(sm.begin(), sm.end()));
}

TEST(FlatMap, ExistingKey) {
    flat_map<unsigned, std::string> myMap{{1, "One"}, {2, "Two"}, {3, "Three"}};

    EXPECT_EQ(get(myMap, 1), "One");
    EXPECT_EQ(get(myMap, 2), "Two");
    EXPECT_EQ(get(myMap, 3), "Three");
}

TEST(FlatMap, NonExistingKey) {
    flat_map<unsigned, std::string> myMap{{1, "One"}, {2, "Two"}, {3, "Three"}};

    EXPECT_EQ(get(myMap, 4), "");
}

}  // namespace P4::Test
