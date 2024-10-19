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

namespace PostMidendConstantFoldingTest {

inline auto defs = R"(
    match_kind {exact}
    header H { bit<32> f1; }
    struct headers_t { H h; }
    struct local_metadata_t {} )";

// We only need to run TableAllocPass (viz DecidePlacement & TransformTables)
// But we will run the FullBackend and verify the ASM generated.
#define RUN_CHECK(input, expected)                                                                \
    do {                                                                                          \
        auto blk = TestCode(TestCode::Hdr::Tofino1arch, TestCode::tofino_shell(),                 \
                            {PostMidendConstantFoldingTest::defs, TestCode::empty_state(), input, \
                             TestCode::empty_appy()},                                             \
                            TestCode::tofino_shell_control_marker());                             \
        blk.flags(Match::TrimWhiteSpace | Match::TrimAnnotations);                                \
        EXPECT_TRUE(blk.CreateBackend());                                                         \
        auto res = blk.match(expected);                                                           \
        EXPECT_TRUE(res.success) << "    @ expected[" << res.count << "], char pos=" << res.pos   \
                                 << "\n"                                                          \
                                 << "    '" << expected[res.count] << "'\n"                       \
                                 << "    '" << blk.extract_code(res.pos) << "'\n";                \
    } while (0)

}  // namespace PostMidendConstantFoldingTest

TEST(PostMidendConstantFolding, SizeInBytesConstConst) {
    auto input = R"(
            apply {
                hdr.h.f1 = sizeInBytes(hdr.h.f1) + 10 + 20;
            }
        )";
    Match::CheckList expected{"action p4headers_tofino1l`(\\d+)`() {",
                              "hdr.h.f1 = 32w34;",
                              "}",
                              "table tbl_p4headers_tofino1l`\\1` {",
                              "actions = {",
                              "p4headers_tofino1l`\\1`();",
                              "}",
                              "const default_action = p4headers_tofino1l`\\1`();",
                              "}",
                              "apply {",
                              "tbl_p4headers_tofino1l`\\1`.apply();",
                              "}"};
    RUN_CHECK(input, expected);
}

TEST(PostMidendConstantFolding, ConstSizeInBytesConst) {
    auto input = R"(
            apply {
                hdr.h.f1 = 10 + sizeInBytes(hdr.h.f1) + 20;
            }
        )";
    Match::CheckList expected{"action p4headers_tofino1l`(\\d+)`() {",
                              "hdr.h.f1 = 32w34;",
                              "}",
                              "table tbl_p4headers_tofino1l`\\1` {",
                              "actions = {",
                              "p4headers_tofino1l`\\1`();",
                              "}",
                              "const default_action = p4headers_tofino1l`\\1`();",
                              "}",
                              "apply {",
                              "tbl_p4headers_tofino1l`\\1`.apply();",
                              "}"};
    RUN_CHECK(input, expected);
}

TEST(PostMidendConstantFolding, ConstConstSizeInBytes) {
    auto input = R"(
            apply {
                hdr.h.f1 = 10 + 20 + sizeInBytes(hdr.h.f1);
            }
        )";
    Match::CheckList expected{"action p4headers_tofino1l`(\\d+)`() {",
                              "hdr.h.f1 = 32w34;",
                              "}",
                              "table tbl_p4headers_tofino1l`\\1` {",
                              "actions = {",
                              "p4headers_tofino1l`\\1`();",
                              "}",
                              "const default_action = p4headers_tofino1l`\\1`();",
                              "}",
                              "apply {",
                              "tbl_p4headers_tofino1l`\\1`.apply();",
                              "}"};
    RUN_CHECK(input, expected);
}

TEST(PostMidendConstantFolding, SizeInBytesConstConstExplicitParentheses) {
    auto input = R"(
            apply {
                hdr.h.f1 = (sizeInBytes(hdr.h.f1) + 10) + 20;
            }
        )";
    Match::CheckList expected{"action p4headers_tofino1l`(\\d+)`() {",
                              "hdr.h.f1 = 32w34;",
                              "}",
                              "table tbl_p4headers_tofino1l`\\1` {",
                              "actions = {",
                              "p4headers_tofino1l`\\1`();",
                              "}",
                              "const default_action = p4headers_tofino1l`\\1`();",
                              "}",
                              "apply {",
                              "tbl_p4headers_tofino1l`\\1`.apply();",
                              "}"};
    RUN_CHECK(input, expected);
}

// Keep definition of RUN_CHECK local for unity builds.
#undef RUN_CHECK

}  // namespace P4::Test
