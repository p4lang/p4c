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

namespace HiddenMergedTest {

inline auto defs = R"(
    match_kind {exact}
    header H { bit<16> f1; bit<16> f2; bit<16> f3;}
    struct headers_t { H h; }
    struct local_metadata_t {} )";

// We only need to run TableAllocPass (viz DecidePlacement & TransformTables)
// But we will run the FullBackend and verify the ASM generated.
#define RUN_CHECK(input, expected)                                                            \
    do {                                                                                      \
        auto blk = TestCode(                                                                  \
            TestCode::Hdr::TofinoMin, TestCode::tofino_shell(),                               \
            {HiddenMergedTest::defs, TestCode::empty_state(), input, TestCode::empty_appy()}, \
            TestCode::tofino_shell_control_marker(), {"--no-dead-code-elimination"});         \
        EXPECT_TRUE(blk.CreateBackend());                                                     \
        EXPECT_TRUE(blk.apply_pass(TestCode::Pass::FullBackend));                             \
        auto res = blk.match(TestCode::CodeBlock::MauAsm, expected);                          \
        EXPECT_TRUE(res.success) << " pos=" << res.pos << " count=" << res.count << "\n'"     \
                                 << blk.extract_code(TestCode::CodeBlock::MauAsm) << "'\n";   \
    } while (0)

}  // namespace HiddenMergedTest

// N.B. The placement runs faster when there is only one table.

TEST(HiddenMerged, HiddenMerged) {
    auto input = R"(
            action test_action() {
                hdr.h.f1 = 0;
            }
            @hidden table test_table {
                key = { hdr.h.f1 : exact; }
                actions = { test_action; }
                const default_action = test_action();
            }
            apply {
                if (hdr.h.f1 == 0x1) {
                    test_table.apply();
                }
            }
        )";

    Match::CheckList expected{"`.*`", "exact_match test_table_`\\d+``.*`:", "`.*`",
                              "p4: {`.*`name: ingress_control.test_table,`.*`hidden: true`.*`}"};
    RUN_CHECK(input, expected);
}

// Keep definition of RUN_CHECK local for unity builds.
#undef RUN_CHECK

}  // namespace P4::Test
