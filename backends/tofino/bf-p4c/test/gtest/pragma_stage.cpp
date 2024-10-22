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

namespace PragmaStageTest {

inline auto defs = R"(
    match_kind {exact}
    header H { bit<16> f1; bit<16> f2; bit<16> f3;}
    struct headers_t { H h; }
    struct local_metadata_t {} )";

// We only need to run TableAllocPass (viz DecidePlacement & TransformTables)
// But we will run the FullBackend and verify the ASM generated.
#define RUN_CHECK(input, expected)                                                           \
    do {                                                                                     \
        auto blk = TestCode(                                                                 \
            TestCode::Hdr::TofinoMin, TestCode::tofino_shell(),                              \
            {PragmaStageTest::defs, TestCode::empty_state(), input, TestCode::empty_appy()}, \
            TestCode::tofino_shell_control_marker(), {"--no-dead-code-elimination"});        \
        EXPECT_TRUE(blk.CreateBackend());                                                    \
        EXPECT_TRUE(blk.apply_pass(TestCode::Pass::FullBackend));                            \
        auto res = blk.match(TestCode::CodeBlock::MauAsm, expected);                         \
        EXPECT_TRUE(res.success) << " pos=" << res.pos << " count=" << res.count << "\n'"    \
                                 << blk.extract_code(TestCode::CodeBlock::MauAsm) << "'\n";  \
    } while (0)

}  // namespace PragmaStageTest

// N.B. The placement runs faster when there is only one table.

TEST(PragmaStage, PragmaStage) {
    auto input = R"(
            action test_action(bit<16> v) {
                hdr.h.f2 = v;
            }

            @stage(1, 4096, "noimmediate")
            @stage(2, 4096, "immediate")
            @stage(3, 4096, "noimmediate")
            @pack(1)
            table test_table {
                key = { hdr.h.f1 : exact; }
                actions = { test_action; }
                const default_action = test_action(0);
                size = 12*1024;
            }
            apply {
                test_table.apply();
            }
        )";

    Match::CheckList expected{
        "`.*`", "stage 1 ingress:",
        "`.*`", "exact_match test_table_`\\d+``.*`:",
        "`.*`", "action test_table_`\\d+``.*`:",
        "`.*`", "stage 2 ingress:",
        "`.*`", "exact_match test_table_`\\d+`$st1`.*`:",
        "`.*`", "format:`.*`immediate`.*`",
        "`.*`", "stage 3 ingress:",
        "`.*`", "exact_match test_table_`\\d+`$st2`.*`:",
        "`.*`", "action test_table_`\\d+`$st2`.*`:",
        "`.*`",
    };
    RUN_CHECK(input, expected);
}

// Keep definition of RUN_CHECK local for unity builds.
#undef RUN_CHECK

}  // namespace P4::Test
