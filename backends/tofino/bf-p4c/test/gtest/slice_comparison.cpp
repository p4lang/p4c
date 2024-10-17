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
