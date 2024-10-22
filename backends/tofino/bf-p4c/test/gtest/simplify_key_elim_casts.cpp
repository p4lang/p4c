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

/*
 * This test tests whether the midend produces suitable output
 * for cast, slice, and concat statements used in the match key.
 * For this purpose, it fires the P4::SimplifyKeys and BFN::ElimCasts
 * passes.
 */

#include "bf-p4c/midend/elim_cast.h"
#include "bf-p4c/midend/simplify_key_policy.h"
#include "bf-p4c/midend/type_checker.h"
#include "bf_gtest_helpers.h"
#include "frontends/common/constantFolding.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeMap.h"
#include "gtest/gtest.h"
#include "midend/simplifyKey.h"

namespace P4::Test {

namespace SimplifyKeyElimCastsTests {

inline auto defs = R"(
    match_kind {exact, ternary, lpm}
    @noWarn("unused") action NoAction() {}
    header Hdr { bit<8> field1; bit<8> field2; bit<16> field3; bit<32> field4;}
    struct Headers { Hdr h; })";

#define RUN_CHECK(pass, input, expected)                                                        \
    do {                                                                                        \
        auto blk = TestCode::TestControlBlock(SimplifyKeyElimCastsTests::defs, input);          \
        blk.flags(Match::TrimWhiteSpace | Match::TrimAnnotations);                              \
        EXPECT_TRUE(blk.apply_pass(TestCode::Pass::FullFrontend));                              \
        EXPECT_TRUE(blk.apply_pass(pass));                                                      \
        auto res = blk.match(expected);                                                         \
        EXPECT_TRUE(res.success) << "    @ expected[" << res.count << "], char pos=" << res.pos \
                                 << "\n"                                                        \
                                 << "    '" << expected[res.count] << "'\n"                     \
                                 << "    '" << blk.extract_code(res.pos) << "'\n";              \
    } while (0)

Visitor *setup_passes() {
    auto refMap = new P4::ReferenceMap;
    auto typeMap = new P4::TypeMap;
    auto typeChecking = new BFN::TypeChecking(refMap, typeMap);
    return new PassManager{
        new P4::SimplifyKey(typeMap, BFN::KeyIsSimple::getPolicy(*typeMap), typeChecking),
        new BFN::ElimCasts(refMap, typeMap), new P4::ConstantFolding(typeMap, true, typeChecking)};
}

}  // namespace SimplifyKeyElimCastsTests

TEST(SimplifyKeyElimCasts, KeyElementSlice) {
    auto input = R"(
        table dummy_table {
            key = {
                headers.h.field4[9:0]: exact @name("key");
            }
            actions = {
            }
        }
        apply {
            dummy_table.apply();
        })";
    Match::CheckList expected = {"action NoAction_`(\\d+)`() { }",
                                 "table dummy_table_`(\\d+)` {",
                                 "key = {",
                                 "headers.h.field4[9:0]: exact ;",
                                 "}",
                                 "actions = {",
                                 "NoAction_`\\1`();",
                                 "}",
                                 "default_action = NoAction_`\\1`();",
                                 "}",
                                 "apply {",
                                 "dummy_table_`\\2`.apply();",
                                 "}"};
    RUN_CHECK(SimplifyKeyElimCastsTests::setup_passes(), input, expected);
}

TEST(SimplifyKeyElimCasts, KeyElementNarrowingCast) {
    auto input = R"(
        table dummy_table {
            key = {
                (bit<12>)headers.h.field4: exact @name("key");
            }
            actions = {
            }
        }
        apply {
            dummy_table.apply();
        })";
    Match::CheckList expected = {"action NoAction_`(\\d+)`() { }",
                                 "bit<12> key_`(\\d+)`;",
                                 "table dummy_table_`(\\d+)` {",
                                 "key = {",
                                 "key_`\\2`: exact ;",
                                 "}",
                                 "actions = {",
                                 "NoAction_`\\1`();",
                                 "}",
                                 "default_action = NoAction_`\\1`();",
                                 "}",
                                 "apply {",
                                 "key_`\\2` = headers.h.field4[11:0];",
                                 "dummy_table_`\\3`.apply();",
                                 "}"};
    RUN_CHECK(SimplifyKeyElimCastsTests::setup_passes(), input, expected);
}

TEST(SimplifyKeyElimCasts, KeyElementWideningCast) {
    auto input = R"(
        table dummy_table {
            key = {
                (bit<63>)headers.h.field4: exact @name("key");
            }
            actions = {
            }
        }
        apply {
            dummy_table.apply();
        })";
    Match::CheckList expected = {"action NoAction_`(\\d+)`() { }",
                                 "bit<63> key_`(\\d+)`;",
                                 "table dummy_table_`(\\d+)` {",
                                 "key = {",
                                 "key_`\\2`: exact ;",
                                 "}",
                                 "actions = {",
                                 "NoAction_`\\1`();",
                                 "}",
                                 "default_action = NoAction_`\\1`();",
                                 "}",
                                 "apply {",
                                 "{",
                                 "bit<31> $concat_to_slice`(\\d+)`;",
                                 "bit<32> $concat_to_slice`(\\d+)`;",
                                 "$concat_to_slice`\\4` = 31w0;",
                                 "$concat_to_slice`\\5` = headers.h.field4;",
                                 "key_`\\2`[62:32] = $concat_to_slice`\\4`;",
                                 "key_`\\2`[31:0] = $concat_to_slice`\\5`;",
                                 "}",
                                 "dummy_table_`\\3`.apply();",
                                 "}"};
    RUN_CHECK(SimplifyKeyElimCastsTests::setup_passes(), input, expected);
}

TEST(SimplifyKeyElimCasts, KeyElementWideningCastSlice) {
    auto input = R"(
        table dummy_table {
            key = {
                ((bit<256>)headers.h.field4)[62:0]: exact @name("key");
            }
            actions = {
            }
        }
        apply {
            dummy_table.apply();
        })";
    Match::CheckList expected = {"action NoAction_`(\\d+)`() { }",
                                 "bit<63> key_`(\\d+)`;",
                                 "table dummy_table_`(\\d+)` {",
                                 "key = {",
                                 "key_`\\2`: exact ;",
                                 "}",
                                 "actions = {",
                                 "NoAction_`\\1`();",
                                 "}",
                                 "default_action = NoAction_`\\1`();",
                                 "}",
                                 "apply {",
                                 "{",
                                 "bit<31> $concat_to_slice`(\\d+)`;",
                                 "bit<32> $concat_to_slice`(\\d+)`;",
                                 "$concat_to_slice`\\4` = 31w0;",
                                 "$concat_to_slice`\\5` = headers.h.field4;",
                                 "key_`\\2`[62:32] = $concat_to_slice`\\4`;",
                                 "key_`\\2`[31:0] = $concat_to_slice`\\5`;",
                                 "}",
                                 "dummy_table_`\\3`.apply();",
                                 "}"};
    RUN_CHECK(SimplifyKeyElimCastsTests::setup_passes(), input, expected);
}

TEST(SimplifyKeyElimCasts, KeyElementWideningCastSliceNonzeroLsb) {
    auto input = R"(
        table dummy_table {
            key = {
                ((bit<256>)headers.h.field4)[62:15]: exact @name("key");
            }
            actions = {
            }
        }
        apply {
            dummy_table.apply();
        })";
    Match::CheckList expected = {"action NoAction_`(\\d+)`() { }",
                                 "bit<48> key_`(\\d+)`;",
                                 "table dummy_table_`(\\d+)` {",
                                 "key = {",
                                 "key_`\\2`: exact ;",
                                 "}",
                                 "actions = {",
                                 "NoAction_`\\1`();",
                                 "}",
                                 "default_action = NoAction_`\\1`();",
                                 "}",
                                 "apply {",
                                 "{",
                                 "bit<31> $concat_to_slice`(\\d+)`;",
                                 "bit<17> $concat_to_slice`(\\d+)`;",
                                 "$concat_to_slice`\\4` = 31w0;",
                                 "$concat_to_slice`\\5` = headers.h.field4[31:15];",
                                 "key_`\\2`[47:17] = $concat_to_slice`\\4`;",
                                 "key_`\\2`[16:0] = $concat_to_slice`\\5`;",
                                 "}",
                                 "dummy_table_`\\3`.apply();",
                                 "}"};
    RUN_CHECK(SimplifyKeyElimCastsTests::setup_passes(), input, expected);
}

TEST(SimplifyKeyElimCasts, KeyElementConcatConstantPathExpression) {
    auto input = R"(
        table dummy_table {
            key = {
                10w0 ++ headers.h.field4: exact @name("key");
            }
            actions = {
            }
        }
        apply {
            dummy_table.apply();
        })";
    Match::CheckList expected = {"action NoAction_`(\\d+)`() { }",
                                 "bit<42> key_`(\\d+)`;",
                                 "table dummy_table_`(\\d+)` {",
                                 "key = {",
                                 "key_`\\2`: exact ;",
                                 "}",
                                 "actions = {",
                                 "NoAction_`\\1`();",
                                 "}",
                                 "default_action = NoAction_`\\1`();",
                                 "}",
                                 "apply {",
                                 "{",
                                 "bit<10> $concat_to_slice`(\\d+)`;",
                                 "bit<32> $concat_to_slice`(\\d+)`;",
                                 "$concat_to_slice`\\4` = 10w0;",
                                 "$concat_to_slice`\\5` = headers.h.field4;",
                                 "key_`\\2`[41:32] = $concat_to_slice`\\4`;",
                                 "key_`\\2`[31:0] = $concat_to_slice`\\5`;",
                                 "}",
                                 "dummy_table_`\\3`.apply();",
                                 "}"};
    RUN_CHECK(SimplifyKeyElimCastsTests::setup_passes(), input, expected);
}

TEST(SimplifyKeyElimCasts, KeyElementConcatPathExpressions) {
    auto input = R"(
        table dummy_table {
            key = {
                headers.h.field1 ++ headers.h.field4: exact @name("key");
            }
            actions = {
            }
        }
        apply {
            dummy_table.apply();
        })";
    Match::CheckList expected = {"action NoAction_`(\\d+)`() { }",
                                 "bit<40> key_`(\\d+)`;",
                                 "table dummy_table_`(\\d+)` {",
                                 "key = {",
                                 "key_`\\2`: exact ;",
                                 "}",
                                 "actions = {",
                                 "NoAction_`\\1`();",
                                 "}",
                                 "default_action = NoAction_`\\1`();",
                                 "}",
                                 "apply {",
                                 "{",
                                 "bit<8> $concat_to_slice`(\\d+)`;",
                                 "bit<32> $concat_to_slice`(\\d+)`;",
                                 "$concat_to_slice`\\4` = headers.h.field1;",
                                 "$concat_to_slice`\\5` = headers.h.field4;",
                                 "key_`\\2`[39:32] = $concat_to_slice`\\4`;",
                                 "key_`\\2`[31:0] = $concat_to_slice`\\5`;",
                                 "}",
                                 "dummy_table_`\\3`.apply();",
                                 "}"};
    RUN_CHECK(SimplifyKeyElimCastsTests::setup_passes(), input, expected);
}

// Keep definition of RUN_CHECK local for unity builds.
#undef RUN_CHECK

}  // namespace P4::Test
