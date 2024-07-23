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

#include "lib/hvec_set.h"

#include <gtest/gtest.h>

namespace Test {

TEST(hvec_set, map_equal) {
    hvec_set<unsigned> a;
    hvec_set<unsigned> b;

    EXPECT_TRUE(a == b);

    a.insert(111);
    a.insert(222);
    a.insert(333);
    a.insert(444);

    b.insert(111);
    b.insert(222);
    b.insert(333);
    b.insert(444);

    EXPECT_TRUE(a == b);

    a.erase(2);
    b.erase(2);

    EXPECT_TRUE(a == b);

    a.clear();
    b.clear();

    EXPECT_TRUE(a == b);
}

TEST(hvec_set, map_not_equal) {
    hvec_set<unsigned> a;
    hvec_set<unsigned> b;

    EXPECT_TRUE(a == b);

    a.insert(111);
    a.insert(222);
    a.insert(333);
    a.insert(444);

    b.insert(444);
    b.insert(333);
    b.insert(222);
    b.insert(111);

    EXPECT_TRUE(a != b);

    a.clear();
    b.clear();

    EXPECT_TRUE(a == b);

    a.insert(111);
    a.insert(222);

    b.insert(111);
    b.insert(222);
    b.insert(333);

    EXPECT_TRUE(a != b);

    a.clear();
    b.clear();

    EXPECT_TRUE(a == b);

    a.insert(111);
    a.insert(222);
    a.insert(333);
    a.insert(444);

    b.insert(111);
    b.insert(222);
    b.insert(333);
    b.insert(555);

    EXPECT_TRUE(a != b);

    a.clear();
    b.clear();

    EXPECT_TRUE(a == b);

    a.insert(111);
    a.insert(222);
    a.insert(333);
    a.insert(444);

    b.insert(111);
    b.insert(111);
    b.insert(111);
    b.insert(111);

    EXPECT_TRUE(a != b);
}

TEST(hvec_set, insert_emplace_erase) {
    hvec_set<unsigned> om;
    std::set<unsigned> sm;

    typename hvec_set<unsigned>::const_iterator it = om.end();
    for (auto v : {0, 1, 2, 3, 4, 5, 6, 7, 8}) {
        sm.emplace(v);
        if (v % 2 == 0) {
            if ((v / 2) % 2 == 0) {
                it = om.insert(v).first;
            } else {
                it = om.emplace(v).first;
            }
        } else {
            unsigned tmp = v;
            if ((v / 2) % 2 == 0) {
                it = om.insert(std::move(tmp)).first;
            } else {
                it = om.emplace(std::move(tmp)).first;
            }
        }
    }
    EXPECT_TRUE(std::next(it) == om.end());

    EXPECT_TRUE(std::equal(om.begin(), om.end(), sm.begin(), sm.end()));

    it = std::next(om.begin(), 2);
    om.erase(it);
    sm.erase(std::next(sm.begin(), 2));

    EXPECT_TRUE(om.size() == sm.size());
    EXPECT_TRUE(std::equal(om.begin(), om.end(), sm.begin(), sm.end()));
}

TEST(hvec_set, string_set) {
    hvec_set<std::string> m, m1;

    for (int i = 1; i <= 100; ++i) {
        m.insert("test" + std::to_string(i));
    }
    m1 = m;

    hvec_set<std::string> m2(m);

    EXPECT_EQ(m1.size(), 100);
    for (int i = 1; i <= 100; i += 2) m1.erase("test" + std::to_string(i));
    EXPECT_EQ(m1.size(), 50);
    for (int i = 102; i <= 200; i += 2) m1.insert(std::to_string(i) + "foobar");
    EXPECT_EQ(m1.size(), 100);

    int idx = 2;
    for (auto &el : m1) {
        if (idx <= 100)
            EXPECT_TRUE(el.c_str() + 4 == std::to_string(idx));
        else
            EXPECT_EQ(el, std::to_string(idx) + "foobar");
        idx += 2;
    }
}

}  // namespace Test
