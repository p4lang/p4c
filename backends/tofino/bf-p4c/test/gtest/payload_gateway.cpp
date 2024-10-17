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

namespace PayloadGatewayTest {

inline auto defs = R"(
    match_kind {ternary}
    header H { bit<16> f1; bit<16> f2; bit<16> f3;}
    struct headers_t { H h; }
    struct local_metadata_t {} )";

// We only need to run TableAllocPass (viz DecidePlacement & TransformTables)
// But we will run the FullBackend and verify the ASM generated.
#define RUN_CHECK(input, expected)                                                              \
    do {                                                                                        \
        auto blk = TestCode(                                                                    \
            TestCode::Hdr::TofinoMin, TestCode::tofino_shell(),                                 \
            {PayloadGatewayTest::defs, TestCode::empty_state(), input, TestCode::empty_appy()}, \
            TestCode::tofino_shell_control_marker());                                           \
        EXPECT_TRUE(blk.CreateBackend());                                                       \
        EXPECT_TRUE(blk.apply_pass(TestCode::Pass::FullBackend));                               \
        auto res = blk.match(TestCode::CodeBlock::MauAsm, expected);                            \
        EXPECT_TRUE(res.success) << "    @ expected[" << res.count << "], char pos=" << res.pos \
                                 << "\n"                                                        \
                                 << "    '" << expected[res.count] << "'\n"                     \
                                 << "    '"                                                     \
                                 << blk.extract_code(TestCode::CodeBlock::MauAsm, res.pos)      \
                                 << "'\n";                                                      \
    } while (0)

}  // namespace PayloadGatewayTest

// N.B. The placement runs faster when there is only one table.

TEST(PayloadGateway, KeyValueMask) {
    auto input = R"(
            action NoAction() {}
            action hit() {}
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

TEST(PayloadGateway, KeyValueMaskSingleBit) {
    auto input = R"(
            action NoAction() {}
            action hit() {}
            table tbl {
                actions = {hit;}
                key = { hdr.h.f1 : ternary; }
                const entries = { 0x8000 &&& 0x8000 : hit(); }
                size = 512;
            }
            apply {
                tbl.apply();
            }
        )";

    Match::CheckList expected{
        "`.*`",        "exact_match", "`.*`",
        "gateway:",    "`.*`",        "match: { 0: hdr.h.f1(0..7), 8: hdr.h.f1(8..15) }",
        "0o1*****:",
        "action: hit",
        "next: END",  // 0x8000
        "miss:",
        "run_table: true"};
    RUN_CHECK(input, expected);
}

TEST(PayloadGateway, DoubleKeyValueMask) {
    auto input = R"(
            action NoAction() {}
            action hit() {}
            table tbl {
                actions = {hit;}
                key = { hdr.h.f1 : ternary; hdr.h.f2 : ternary; }
                const entries = { (1 &&& 7, 2 &&& 7) : hit(); }
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
                              "match: { 0: hdr.h.f2(0..7),",  // 0b*****010
                              "8: hdr.h.f2(8..15),",          // 0b********
                              "16: hdr.h.f1(0..7),",          // 0b*****001
                              "24: hdr.h.f1(8..15) }",        // 0b********
                              "0b*************001*************010:",
                              "action: hit",
                              "next: END",                    // (1 &&& 7, 2 &&& 7)
                              "miss:",
                              "run_table: true"};
    RUN_CHECK(input, expected);
}

TEST(PayloadGateway, TooBigToFit) {
    auto input = R"(
            action NoAction() {}
            action hit() {}
            table tbl {
                actions = {hit;}
                key = { hdr.h.f1 : ternary; hdr.h.f2 : ternary; hdr.h.f3 : ternary; }
                const entries = { (1 &&& 7, 2 &&& 7, 4 &&& 7) : hit(); }
                size = 512;
            }
            apply {
                tbl.apply();
            }
        )";

    Match::CheckList expected{
        "`.*`",
        "ternary_match",
        "`.*`",
        "match_key_fields_values:",
        "- field_name: hdr.h.f1",
        "value: \"0x1\"",
        "mask: \"0x7\"",
        "- field_name: hdr.h.f2",
        "value: \"0x2\"",
        "mask: \"0x7\"",
        "- field_name: hdr.h.f3",
        "value: \"0x4\"",
        "mask: \"0x7\"",
    };
    RUN_CHECK(input, expected);
}

// Keep definition of RUN_CHECK local for unity builds.
#undef RUN_CHECK

}  // namespace P4::Test
