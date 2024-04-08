/*
Copyright 2024 Intel Corp.

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

#include "lib/map.h"

#include <gtest/gtest.h>

#include <algorithm>

namespace Test {

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

}  // namespace Test
