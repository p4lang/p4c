/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bf-p4c/common/slice.h"

#include "gtest/gtest.h"

namespace P4::Test {

TEST(slice, MakeSlice) {
    // original bug: bit<128> tmp = ((bit<128>)(h.h.a) | 128w1551532840);
    // which becomes: 0[0..63] ++ h.h.a[0..63] | 1551532840[0..127];
    // and then bits[0..63] are sliced.
    auto field = new IR::Constant(IR::Type::Bits::get(64), 0xdeadbeefdeadbeef);
    auto before = new IR::BOr(
        new IR::Concat(IR::Type::Bits::get(128), new IR::Constant(IR::Type::Bits::get(64), 0),
                       new IR::Member(IR::Type::Bits::get(64), field, "field")),
        new IR::Constant(IR::Type::Bits::get(128), 1551532840));

    auto after = MakeSlice(before, 0, 63);

    auto expected = new IR::BOr(new IR::Member(IR::Type::Bits::get(64), field, "field"),
                                new IR::Constant(IR::Type::Bits::get(64), 1551532840));

    ASSERT_FALSE(before->equiv(*expected));
    ASSERT_TRUE(after->equiv(*expected));
}

}  // namespace P4::Test
