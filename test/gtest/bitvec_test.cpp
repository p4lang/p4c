
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
#include "lib/bitvec.h"

namespace Test {

TEST(Bitvec, Shift) {
    bitvec simple(0, 1);

    EXPECT_EQ((simple << 64).getbit(64), true);
    EXPECT_EQ((simple << 63).getbit(63), true);
    EXPECT_EQ((simple << 64).getbit(63), false);
}

}  // namespace Test
