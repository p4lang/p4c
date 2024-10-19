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

namespace PragmaNoGatewayConversionTest {

inline auto defs = R"(
    match_kind {ternary}
    header H { bit<16> f1; bit<16> f2; bit<16> f3;}
    struct headers_t { H h; }
    struct local_metadata_t {} )";

// We only need to run TableAllocPass (viz DecidePlacement & TransformTables)
// But we will run the FullBackend and verify the ASM generated.
#define RUN_CHECK(input, expected)                                                                \
    do {                                                                                          \
        auto blk = TestCode(TestCode::Hdr::TofinoMin, TestCode::tofino_shell(),                   \
                            {PragmaNoGatewayConversionTest::defs, TestCode::empty_state(), input, \
                             TestCode::empty_appy()},                                             \
                            TestCode::tofino_shell_control_marker());                             \
        EXPECT_TRUE(blk.CreateBackend());                                                         \
        EXPECT_TRUE(blk.apply_pass(TestCode::Pass::FullBackend));                                 \
        auto res = blk.match(TestCode::CodeBlock::MauAsm, expected);                              \
        EXPECT_TRUE(res.success) << "    @ expected[" << res.count << "], char pos=" << res.pos   \
                                 << "\n"                                                          \
                                 << "    '" << expected[res.count] << "'\n"                       \
                                 << "    '"                                                       \
                                 << blk.extract_code(TestCode::CodeBlock::MauAsm, res.pos)        \
                                 << "'\n";                                                        \
    } while (0)

}  // namespace PragmaNoGatewayConversionTest

// N.B. The placement runs faster when there is only one table.

TEST(PragmaNoGatewayConversion, WithoutPragma) {
    auto input = R"(
            action NoAction() {}
            action hit() {}
            // @pragma no_gateway_conversion
            table tbl {
                actions = {hit;}
                key = { hdr.h.f1 : ternary; }
                const entries = { (1 &&& 7) : hit(); }
                size = 512;
            }
            apply {
                tbl.apply();
            }
        )";

    Match::CheckList expected{"`.*`",
                              "exact_match",
                              "`.*`",
                              "gateway:",
                              "`.*`",
                              "match: { 0: hdr.h.f1(0..7), 8: hdr.h.f1(8..15) }",
                              "0b*************001:",
                              "action: hit",
                              "next: END",  // (1 &&& 7)
                              "miss:",
                              "run_table: true"};
    RUN_CHECK(input, expected);
}

TEST(PragmaNoGatewayConversion, WithPragma) {
    auto input = R"(
            action NoAction() {}
            action hit() {}
            @pragma no_gateway_conversion
            table tbl {
                actions = {hit;}
                key = { hdr.h.f1 : ternary; }
                const entries = { (1 &&& 7) : hit(); }
                size = 512;
            }
            apply {
                tbl.apply();
            }
        )";

    Match::CheckList expected{
        "`.*`", "ternary_match",
        "`.*`", "p4: { name: ingress_control.tbl, size: `\\d*` }",
        "`.*`", "match: - { group: `\\d*`, byte_config: `\\d*`, dirtcam: 0x`\\d*` }",
        "`.*`", "miss: END"};
    RUN_CHECK(input, expected);
}

// Keep definition of RUN_CHECK local for unity builds.
#undef RUN_CHECK

}  // namespace P4::Test
