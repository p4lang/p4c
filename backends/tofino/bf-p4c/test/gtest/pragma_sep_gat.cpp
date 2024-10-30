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

namespace PragmaSepGatTest {

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
            {PragmaSepGatTest::defs, TestCode::empty_state(), input, TestCode::empty_appy()}, \
            TestCode::tofino_shell_control_marker(), {"--no-dead-code-elimination"});         \
        EXPECT_TRUE(blk.CreateBackend());                                                     \
        EXPECT_TRUE(blk.apply_pass(TestCode::Pass::FullBackend));                             \
        auto res = blk.match(TestCode::CodeBlock::MauAsm, expected);                          \
        EXPECT_TRUE(res.success) << " pos=" << res.pos << " count=" << res.count << "\n'"     \
                                 << blk.extract_code(TestCode::CodeBlock::MauAsm) << "'\n";   \
    } while (0)

}  // namespace PragmaSepGatTest

// N.B. The placement runs faster when there is only one table.

TEST(PragmaSepGat, PragmaSepGat) {
    auto input = R"(
            action nop() { }
            action set_f1(bit<16> param) { hdr.h.f1 = param; }
            action set_f2(bit<16> param) { hdr.h.f2 = param; }
            action set_f3(bit<16> param) { hdr.h.f3 = param; }

            @separate_gateway
            table t1 {
                key = { hdr.h.f1 : exact; }
                actions = { nop; set_f1; }
                const default_action = nop;
                size = 1024;
            }

            table t2 {
                key = { hdr.h.f3 : exact; }
                actions = { nop; set_f2; }
                const default_action = nop;
                size = 1024;
            }

            table t3 {
                key = { hdr.h.f1 : exact; }
                actions = { nop; set_f3; }
                const default_action = nop;
                size = 1024;
            }

            apply{
                if (hdr.h.f2 == 0) {
                    t1.apply();
                    t2.apply();
                    t3.apply();
                }
            }
        )";

    Match::CheckList expected{
        "`.*`", "stage 0 ingress:",           "`.*`", "exact_match t2_`\\d+``.*`:",
        "`.*`", "stage 1 ingress:",           "`.*`", "dependency: match",
        "`.*`", "exact_match t1_`\\d+``.*`:", "`.*`", "stage 2 ingress:",
        "`.*`", "exact_match t3_`\\d+``.*`:", "`.*`",
    };
    RUN_CHECK(input, expected);
}

TEST(PragmaSepGat, PragmaSepGat2) {
    auto input = R"(
            action nop() { }
            action set_f1(bit<16> param) { hdr.h.f1 = param; }
            action set_f2(bit<16> param) { hdr.h.f2 = param; }
            action set_f3(bit<16> param) { hdr.h.f3 = param; }

            @separate_gateway
            table t1 {
                key = { hdr.h.f1 : exact; }
                actions = { nop; set_f1; }
                const default_action = nop;
                size = 1024;
            }

            @separate_gateway
            table t2 {
                key = { hdr.h.f3 : exact; }
                actions = { nop; set_f2; }
                const default_action = nop;
                size = 1024;
            }

            table t3 {
                key = { hdr.h.f1 : exact; }
                actions = { nop; set_f3; }
                const default_action = nop;
                size = 1024;
            }

            apply{
                if (hdr.h.f2 == 0) {
                    t1.apply();
                    t3.apply();
                } else {
                    t2.apply();
                }
            }
        )";

    Match::CheckList expected{
        "`.*`", "stage 1 ingress:",
        "`.*`", "dependency: match",
        "`.*`", "exact_match t`[1|2]`_`\\d+``.*`:",
        "`.*`", "exact_match t`[1|2]`_`\\d+``.*`:",
        "`.*`", "stage 2 ingress:",
        "`.*`", "exact_match t3_`\\d+``.*`:",
        "`.*`",
    };
    RUN_CHECK(input, expected);
}

// Keep definition of RUN_CHECK local for unity builds.
#undef RUN_CHECK

}  // namespace P4::Test
