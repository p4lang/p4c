/*
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

#include "lib/string_map.h"

#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include <stdexcept>

#include "lib/map.h"
#include "lib/ordered_map.h"

using namespace ::P4C::P4::literals;
using ::testing::ElementsAre;
using ::testing::ElementsAreArray;

namespace P4C::Test {

TEST(StringMap, RangeConstructor) {
    string_map<unsigned>::value_type myMap[] = {{"One"_cs, 1},   {"One"_cs, 2},   {"One"_cs, 3},
                                                {"Two"_cs, 1},   {"Two"_cs, 2},   {"Two"_cs, 3},
                                                {"Three"_cs, 1}, {"Three"_cs, 2}, {"Three"_cs, 3}};

    string_map<unsigned> first(std::begin(myMap), std::end(myMap));
    EXPECT_THAT(first, ElementsAre(std::pair("One"_cs, 1), std::pair("Two"_cs, 1),
                                   std::pair("Three"_cs, 1)));
}

TEST(StringMap, InitializerListConstructor) {
    string_map<unsigned> myMap({{"One"_cs, 1},
                                {"Two"_cs, 2},
                                {"Three"_cs, 3},
                                {"Four"_cs, 4},
                                {"Five"_cs, 5},
                                {"One"_cs, 2},
                                {"Ten"_cs, 10},
                                {"Eight"_cs, 8}});
    EXPECT_THAT(
        myMap, ElementsAre(std::pair("One"_cs, 1), std::pair("Two"_cs, 2), std::pair("Three"_cs, 3),
                           std::pair("Four"_cs, 4), std::pair("Five"_cs, 5),
                           std::pair("Ten"_cs, 10), std::pair("Eight"_cs, 8)));
}

TEST(StringMap, InitializerListAssignment) {
    string_map<unsigned> myMap;
    myMap = {{"One"_cs, 1}, {"Two"_cs, 2}};
    EXPECT_THAT(myMap, ElementsAre(std::pair("One"_cs, 1), std::pair("Two"_cs, 2)));
}

TEST(StringMap, InsertFindSize) {
    string_map<unsigned> s;
    s.insert(std::pair("One"_cs, 1));
    s.insert(std::pair("One"_cs, 1));
    s.insert(std::pair("Two"_cs, 2));

    EXPECT_EQ(2u, s.size());
    EXPECT_EQ(std::pair("One"_cs, 1u), *s.find("One"_cs));
    EXPECT_EQ(std::pair("Two"_cs, 2u), *s.find("Two"_cs));
    EXPECT_EQ(s.end(), s.find("Seven"_cs));
}

TEST(StringMap, CopySwap) {
    string_map<unsigned> original;
    original.insert({"One"_cs, 1});
    original.insert({"Two"_cs, 2});
    EXPECT_THAT(original, ElementsAre(std::pair("One"_cs, 1), std::pair("Two"_cs, 2)));

    string_map<unsigned> copy(original);
    EXPECT_THAT(copy, ElementsAre(std::pair("One"_cs, 1), std::pair("Two"_cs, 2)));

    copy.erase(copy.begin());
    copy.insert({"Ten"_cs, 10});
    EXPECT_THAT(copy, ElementsAre(std::pair("Two"_cs, 2), std::pair("Ten"_cs, 10)));

    original.swap(copy);
    EXPECT_THAT(original, ElementsAre(std::pair("Two"_cs, 2), std::pair("Ten"_cs, 10)));
    EXPECT_THAT(copy, ElementsAre(std::pair("One"_cs, 1), std::pair("Two"_cs, 2)));
}

// operator[](const Key&)
TEST(StringMap, SubscriptConstKey) {
    string_map<unsigned> m;

    // Default construct elements that don't exist yet.
    unsigned &s = m["a"_cs];
    EXPECT_EQ(0, s);
    EXPECT_EQ(1u, m.size());

    // The returned mapped reference should refer to the map.
    s = 22;
    EXPECT_EQ(22, m["a"_cs]);

    // Overwrite existing elements.
    m["a"_cs] = 44;
    EXPECT_EQ(44, m["a"_cs]);

    // Same, but with heterogeneous lookup
    unsigned &s2 = m["b"];
    EXPECT_EQ(0, s2);
    EXPECT_EQ(2u, m.size());

    // The returned mapped reference should refer to the map.
    s2 = 22;
    EXPECT_EQ(22, m["b"]);

    // Overwrite existing elements.
    m["b"] = 44;
    EXPECT_EQ(44, m["b"]);
}

// mapped_type& at(const Key&)
// const mapped_type& at(const Key&) const
TEST(StringMap, At) {
    string_map<unsigned> m = {{"a"_cs, 1}, {"b"_cs, 2}};

    // basic usage.
    EXPECT_EQ(1, m.at("a"_cs));
    EXPECT_EQ(2, m.at("b"_cs));

    // const reference works.
    const unsigned &const_ref = std::as_const(m).at("a"_cs);
    EXPECT_EQ(1, const_ref);

    // reference works
    m.at("a"_cs) = 3;
    EXPECT_EQ(3, m.at("a"_cs));

    // out-of-bounds will throw.
    EXPECT_THROW(m.at("c"_cs), std::out_of_range);
    EXPECT_THROW({ m.at("c"_cs) = 42; }, std::out_of_range);

    // heterogeneous look-up works.
    string_map<unsigned> m2 = {{"a"_cs, 1}, {"b"_cs, 2}};
    EXPECT_EQ(1, m2.at(std::string_view("a")));
    EXPECT_EQ(2, std::as_const(m2).at(std::string_view("b")));
}

TEST(StringMap, InsertEmplaceErase) {
    string_map<unsigned> om;
    ordered_map<cstring, unsigned> sm;

    auto it = om.end();
    for (auto v : {0, 1, 2, 3, 4, 5, 6, 7, 8}) {
        sm.emplace(std::to_string(v), 2 * v);
        std::pair<cstring, unsigned> pair{std::to_string(v), 2 * v};
        if (v % 2 == 0) {
            if ((v / 2) % 2 == 0) {
                it = om.insert(pair).first;
            } else {
                it = om.emplace(std::string_view(std::to_string(v)), pair.second).first;
            }
        } else {
            if ((v / 2) % 2 == 0) {
                it = om.insert(std::move(pair)).first;
            } else {
                it = om.emplace(std::string_view(std::to_string(v)), v * 2).first;
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

TEST(StringMap, MapEqual) {
    string_map<unsigned> a;
    string_map<unsigned> b;

    EXPECT_TRUE(a == b);

    a["1"_cs] = 111;
    a["2"_cs] = 222;
    a["3"_cs] = 333;
    a["4"_cs] = 444;

    b["1"_cs] = 111;
    b["2"_cs] = 222;
    b["3"_cs] = 333;
    b["4"_cs] = 444;

    EXPECT_EQ(a, b);

    a.erase("2"_cs);
    b.erase("2"_cs);

    EXPECT_EQ(a, b);

    a.clear();
    b.clear();

    EXPECT_EQ(a, b);
}

TEST(StringMap, MapNotEqual) {
    string_map<unsigned> a;
    string_map<unsigned> b;

    EXPECT_TRUE(a == b);

    a["1"_cs] = 111;
    a["2"_cs] = 222;
    a["3"_cs] = 333;
    a["4"_cs] = 444;

    b["4"_cs] = 444;
    b["3"_cs] = 333;
    b["2"_cs] = 222;
    b["1"_cs] = 111;

    EXPECT_NE(a, b);

    a.clear();
    b.clear();

    EXPECT_EQ(a, b);

    a["1"_cs] = 111;
    a["2"_cs] = 222;

    b["1"_cs] = 111;
    b["2"_cs] = 222;
    b["3"_cs] = 333;

    EXPECT_NE(a, b);

    a.clear();
    b.clear();

    EXPECT_EQ(a, b);

    a["1"_cs] = 111;
    a["2"_cs] = 222;
    a["3"_cs] = 333;
    a["4"_cs] = 444;

    b["4"_cs] = 111;
    b["3"_cs] = 222;
    b["2"_cs] = 333;
    b["1"_cs] = 444;

    EXPECT_NE(a, b);

    a.clear();
    b.clear();

    EXPECT_EQ(a, b);

    a["1"_cs] = 111;
    a["2"_cs] = 222;
    a["3"_cs] = 333;
    a["4"_cs] = 444;

    b["1"_cs] = 111;
    b["2"_cs] = 111;
    b["3"_cs] = 111;
    b["4"_cs] = 111;

    EXPECT_NE(a, b);
}

TEST(StringMap, ExistingKey) {
    string_map<int> myMap{{"One"_cs, 1}, {"Two"_cs, 2}, {"Three"_cs, 3}};

    EXPECT_EQ(get(myMap, "One"_cs), 1);
    EXPECT_EQ(get(myMap, "Two"_cs), 2);
    EXPECT_EQ(get(myMap, "Three"_cs), 3);
}

TEST(StringMap, NonExistingKey) {
    string_map<int> myMap{{"One"_cs, 1}, {"Two"_cs, 2}, {"Three"_cs, 3}};

    EXPECT_EQ(get(myMap, "Four"_cs), 0);
}

}  // namespace P4C::Test
