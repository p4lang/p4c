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

#include "gtest/gtest.h"
#include "lib/ordered_set.h"

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


}  // namespace Test
