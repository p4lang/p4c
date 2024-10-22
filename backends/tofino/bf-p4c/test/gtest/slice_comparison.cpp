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

#include "bf_gtest_helpers.h"
#include "gtest/gtest.h"

namespace P4::Test {

namespace SliceComparisonTest {

inline auto defs = R"(
    match_kind {exact}
    header H { bit<4> pad1; bit<4> pri1; bit<4> pad2; bit<4> pri2; }
    struct headers_t { H h; }
    struct local_metadata_t {} )";

#define RUN_CHECK(input, expected)                                                               \
    do {                                                                                         \
        auto blk = TestCode(                                                                     \
            TestCode::Hdr::TofinoMin, TestCode::tofino_shell(),                                  \
            {SliceComparisonTest::defs, TestCode::empty_state(), input, TestCode::empty_appy()}, \
            TestCode::tofino_shell_control_marker(), {"--no-dead-code-elimination"});            \
        EXPECT_TRUE(blk.CreateBackend());                                                        \
        EXPECT_TRUE(blk.apply_pass(TestCode::Pass::FullBackend));                                \
        auto res = blk.match(TestCode::CodeBlock::MauAsm, expected);                             \
        EXPECT_TRUE(res.success) << " pos=" << res.pos << " count=" << res.count << "\n'"        \
                                 << blk.extract_code(TestCode::CodeBlock::MauAsm) << "'\n";      \
    } while (0)

}  // namespace SliceComparisonTest

TEST(SliceComparison, SliceComparison1) {
    auto input = R"(
            apply {
                bit<4> pri = hdr.h.pri1;
                bit<4> pri2 = hdr.h.pri2;
                if(pri[3:3] != pri2[3:3] && pri & 8 == 8) {
                    hdr.h.pri1 = hdr.h.pri2;
                }
            }
        )";

    Match::CheckList expected{
        "`.*`",
        "gateway`.*`:",
        "`.*`",
        "match: { 3: hdr.h.pri1(3), 11: hdr.h.pri1(3) }",
        "xor: { 3: hdr.h.pri2(3) }",
        "0b1*******1:",
        "run_table: true",
    };
    RUN_CHECK(input, expected);
}

TEST(SliceComparison, SliceComparison2) {
    auto input = R"(
            apply {
                bit<4> pri = hdr.h.pri1;
                bit<4> pri2 = hdr.h.pri2;
                if(pri[3:3] != pri2[3:3] && pri2 & 8 == 8) {
                    hdr.h.pri1 = hdr.h.pri2;
                }
            }
        )";

    Match::CheckList expected{
        "`.*`",
        "gateway`.*`:",
        "`.*`",
        "match: { 3: hdr.h.pri1(3), 11: hdr.h.pri2(3) }",
        "xor: { 3: hdr.h.pri2(3) }",
        "0b1*******1:",
        "run_table: true",
    };
    RUN_CHECK(input, expected);
}

// Keep definition of RUN_CHECK local for unity builds.
#undef RUN_CHECK

}  // namespace P4::Test
