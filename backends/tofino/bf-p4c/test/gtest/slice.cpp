/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
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
