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
