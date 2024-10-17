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

/*
 * This test covers slicing, downcasting, and upcasting
 * of the output of the read method of a Register extern.
 */

#include "bf-p4c/midend/register_read_write.h"

#include "bf_gtest_helpers.h"
#include "gtest/gtest.h"

#include "bf-p4c/arch/arch.h"
#include "bf-p4c/bf-p4c-options.h"
#include "bf-p4c/midend/action_synthesis_policy.h"
#include "bf-p4c/midend/elim_cast.h"
#include "bf-p4c/midend/type_checker.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/moveDeclarations.h"
#include "frontends/p4/simplify.h"
#include "frontends/p4/typeMap.h"
#include "midend/actionSynthesis.h"

namespace P4::Test {

namespace RegisterReadWriteTest {

inline auto defs = R"(
    struct headers_t { }
    struct local_metadata_t { bit<5> result_5b; bit<16> result_16b; bit<32> result_32b; })";

auto defs2 = R"(
    struct headers_t { }
    struct local_metadata_t { bit<32> result1; bit<32> result2; bit<32> result3; })";

#define RUN_CHECK(defs, pass, input, expected)                                                     \
    do {                                                                                           \
        auto blk = TestCode(                                                                       \
            TestCode::Hdr::Tofino1arch, TestCode::tofino_shell(),                                  \
            {RegisterReadWriteTest::defs, TestCode::empty_state(), input, TestCode::empty_appy()}, \
            TestCode::tofino_shell_control_marker());                                              \
        blk.flags(Match::TrimWhiteSpace | Match::TrimAnnotations);                                 \
        EXPECT_TRUE(blk.apply_pass(TestCode::Pass::FullFrontend));                                 \
        EXPECT_TRUE(blk.apply_pass(pass));                                                         \
        auto res = blk.match(expected);                                                            \
        EXPECT_TRUE(res.success) << "    @ expected[" << res.count << "], char pos=" << res.pos    \
                                 << "\n"                                                           \
                                 << "    '" << expected[res.count] << "'\n"                        \
                                 << "    '" << blk.extract_code(res.pos) << "'\n";                 \
    } while (0)

/*
 * For downcasting and slicing of the result of the read method,
 * only the ArchTranslation and RegisterReadWrite passes are needed.
 * For upcasting of the result of the read method, in addition to the above mentioned passes,
 * the ElimCasts, MoveDeclarations, and SimplifyControlFlow passes are needed.
 * The SynthesizeActions pass is needed for reads/writes placed in an apply block
 * of a control block.
 */
Visitor *setup_passes() {
    auto refMap = new P4::ReferenceMap;
    auto typeMap = new P4::TypeMap;
    auto typeChecking = new BFN::TypeChecking(refMap, typeMap);
    return new PassManager{
        typeChecking,
        new BFN::ArchTranslation(refMap, typeMap, BackendOptions()),
        new BFN::ElimCasts(refMap, typeMap),
        new P4::MoveDeclarations(),
        new P4::SimplifyControlFlow(typeMap, typeChecking),
        new P4::SynthesizeActions(
            refMap, typeMap, new BFN::ActionSynthesisPolicy(new std::set<cstring>, refMap, typeMap),
            typeChecking),
        new BFN::RegisterReadWrite(refMap, typeMap, typeChecking)};
}

}  // namespace RegisterReadWriteTest

TEST(RegisterReadWrite, ReadSlice) {
    auto input = R"(
        Register<bit<16>, PortId_t>(1024) reg_16b;
        action reg_16b_action() {
            ig_md.result_5b = reg_16b.read(ig_intr_md.ingress_port)[4:0];
        }
        apply {
            reg_16b_action();
        })";
    Match::CheckList expected{
        "Register<bit<16>, PortId_t>(32w1024) reg_16b_0;",
        "RegisterAction<bit<16>, PortId_t, bit<16>>(reg_16b_0)",
        "reg_16b_0_reg_16b_action = {",
        "void apply(inout bit<16> value, out bit<16> rv) {",
        "bit<16> in_value;",
        "in_value = value;",
        "rv = value;",
        "}",
        "};",
        "action reg_16b_action() {",
        "ig_md.result_5b = reg_16b_0_reg_16b_action.execute(ig_intr_md.ingress_port)[4:0];",
        "}",
        "apply {",
        "reg_16b_action();",
        "}"};
    RUN_CHECK(defs, RegisterReadWriteTest::setup_passes(), input, expected);
}

TEST(RegisterReadWrite, ReadDowncast) {
    auto input = R"(
        Register<bit<16>, PortId_t>(1024) reg_16b;
        action reg_16b_action() {
            ig_md.result_5b = (bit<5>)reg_16b.read(ig_intr_md.ingress_port);
        }
        apply {
            reg_16b_action();
        })";
    Match::CheckList expected{
        "Register<bit<16>, PortId_t>(32w1024) reg_16b_0;",
        "RegisterAction<bit<16>, PortId_t, bit<16>>(reg_16b_0)",
        "reg_16b_0_reg_16b_action = {",
        "void apply(inout bit<16> value, out bit<16> rv) {",
        "bit<16> in_value;",
        "in_value = value;",
        "rv = value;",
        "}",
        "};",
        "action reg_16b_action() {",
        "ig_md.result_5b = reg_16b_0_reg_16b_action.execute(ig_intr_md.ingress_port)[4:0];",
        "}",
        "apply {",
        "reg_16b_action();",
        "}"};
    RUN_CHECK(defs, RegisterReadWriteTest::setup_passes(), input, expected);
}

TEST(RegisterReadWrite, ReadUpcast) {
    auto input = R"(
        Register<bit<16>, PortId_t>(1024) reg_16b;
        action reg_16b_action() {
            ig_md.result_32b = (bit<32>)reg_16b.read(ig_intr_md.ingress_port);
        }
        apply {
            reg_16b_action();
        })";
    Match::CheckList expected{
        "bit<16> $concat_to_slice`(\\d+)`;",
        "bit<16> $concat_to_slice`(\\d+)`;",
        "Register<bit<16>, PortId_t>(32w1024) reg_16b_0;",
        "RegisterAction<bit<16>, PortId_t, bit<16>>(reg_16b_0)",
        "reg_16b_0_reg_16b_action = {",
        "void apply(inout bit<16> value, out bit<16> rv) {",
        "bit<16> in_value;",
        "in_value = value;",
        "rv = value;",
        "}",
        "};",
        "action reg_16b_action() {",
        "$concat_to_slice`\\1` = 16w0;",
        "$concat_to_slice`\\2` = reg_16b_0_reg_16b_action.execute(ig_intr_md.ingress_port);",
        "ig_md.result_32b[31:16] = $concat_to_slice`\\1`;",
        "ig_md.result_32b[15:0] = $concat_to_slice`\\2`;",
        "}",
        "apply {",
        "reg_16b_action();",
        "}"};
    RUN_CHECK(defs, RegisterReadWriteTest::setup_passes(), input, expected);
}

TEST(RegisterReadWrite, ReadRead) {
    auto input = R"(
        Register<bit<32>, PortId_t>(1024) reg_32b;
        action test_action_1() {
            ig_md.result1 = reg_32b.read(ig_intr_md.ingress_port);
        }
        action test_action_2() {
            ig_md.result2 = reg_32b.read(ig_intr_md.ingress_port);
        }
        table test_table {
            key = {
                ig_intr_md.ingress_port : exact;
            }
            actions = { test_action_1; test_action_2; }
        }
        apply {
            test_table.apply();
        }
    )";
    Match::CheckList expected{
        "action NoAction_1() { }",
        "Register<bit<32>, PortId_t>(32w1024) reg_32b_0;",
        "RegisterAction<bit<32>, PortId_t, bit<32>>(reg_32b_0)",
        "reg_32b_0_test_action_2 = {",
        "void apply(inout bit<32> value, out bit<32> rv) {",
        "bit<32> in_value;",
        "in_value = value;",
        "rv = value;",
        "}",
        "};",
        "RegisterAction<bit<32>, PortId_t, bit<32>>(reg_32b_0)",
        "reg_32b_0_test_action_1 = {",
        "void apply(inout bit<32> value, out bit<32> rv) {",
        "bit<32> in_value;",
        "in_value = value;",
        "rv = value;",
        "}",
        "};",
        "action test_action_1() {",
        "ig_md.result1 = reg_32b_0_test_action_1.execute(ig_intr_md.ingress_port);",
        "}",
        "action test_action_2() {",
        "ig_md.result2 = reg_32b_0_test_action_2.execute(ig_intr_md.ingress_port);",
        "}",
        "table test_table_0 {",
        "key = {",
        " ig_intr_md.ingress_port: exact ; ",
        "}",
        "actions = { test_action_1(); test_action_2(); NoAction_1(); } ",
        "default_action = NoAction_1(); "
        "}",
        "apply {",
        "test_table_0.apply();",
        "}"};
    RUN_CHECK(defs2, RegisterReadWriteTest::setup_passes(), input, expected);
}

TEST(RegisterReadWrite, ReadWrite) {
    auto input = R"(
        Register<bit<32>, PortId_t>(1024) reg_32b;
        action test_action_1() {
            ig_md.result1 = reg_32b.read(ig_intr_md.ingress_port);
        }
        action test_action_2() {
            reg_32b.write(ig_intr_md.ingress_port, 42);
        }
        table test_table {
            key = {
                ig_intr_md.ingress_port : exact;
            }
            actions = { test_action_1; test_action_2; }
        }
        apply {
            test_table.apply();
        }
    )";
    Match::CheckList expected{
        "action NoAction_1() { }",
        "Register<bit<32>, PortId_t>(32w1024) reg_32b_0;",
        "RegisterAction<bit<32>, PortId_t, bit<32>>(reg_32b_0)",
        "reg_32b_0_test_action_2 = {",
        "void apply(inout bit<32> value) {",
        "value = 32w42;",
        "}",
        "};",
        "RegisterAction<bit<32>, PortId_t, bit<32>>(reg_32b_0)",
        "reg_32b_0_test_action_1 = {",
        "void apply(inout bit<32> value, out bit<32> rv) {",
        "bit<32> in_value;",
        "in_value = value;",
        "rv = value;",
        "}",
        "};",
        "action test_action_1() {",
        "ig_md.result1 = reg_32b_0_test_action_1.execute(ig_intr_md.ingress_port);",
        "}",
        "action test_action_2() {",
        "reg_32b_0_test_action_2.execute(ig_intr_md.ingress_port);",
        "}",
        "table test_table_0 {",
        "key = {",
        " ig_intr_md.ingress_port: exact ; ",
        "}",
        "actions = { test_action_1(); test_action_2(); NoAction_1(); } ",
        "default_action = NoAction_1(); "
        "}",
        "apply {",
        "test_table_0.apply();",
        "}"};
    RUN_CHECK(defs2, RegisterReadWriteTest::setup_passes(), input, expected);
}

TEST(RegisterReadWrite, WriteWrite) {
    auto input = R"(
        Register<bit<32>, PortId_t>(1024) reg_32b;
        action test_action_1() {
            reg_32b.write(ig_intr_md.ingress_port, 41);
        }
        action test_action_2() {
            reg_32b.write(ig_intr_md.ingress_port, 42);
        }
        table test_table {
            key = {
                ig_intr_md.ingress_port : exact;
            }
            actions = { test_action_1; test_action_2; }
        }
        apply {
            test_table.apply();
        }
    )";
    Match::CheckList expected{"action NoAction_1() { }",
                              "Register<bit<32>, PortId_t>(32w1024) reg_32b_0;",
                              "RegisterAction<bit<32>, PortId_t, bit<32>>(reg_32b_0)",
                              "reg_32b_0_test_action_2 = {",
                              "void apply(inout bit<32> value) {",
                              "value = 32w42;",
                              "}",
                              "};",
                              "RegisterAction<bit<32>, PortId_t, bit<32>>(reg_32b_0)",
                              "reg_32b_0_test_action_1 = {",
                              "void apply(inout bit<32> value) {",
                              "value = 32w41;",
                              "}",
                              "};",
                              "action test_action_1() {",
                              "reg_32b_0_test_action_1.execute(ig_intr_md.ingress_port);",
                              "}",
                              "action test_action_2() {",
                              "reg_32b_0_test_action_2.execute(ig_intr_md.ingress_port);",
                              "}",
                              "table test_table_0 {",
                              "key = {",
                              " ig_intr_md.ingress_port: exact ; ",
                              "}",
                              "actions = { test_action_1(); test_action_2(); NoAction_1(); } ",
                              "default_action = NoAction_1(); "
                              "}",
                              "apply {",
                              "test_table_0.apply();",
                              "}"};
    RUN_CHECK(defs2, RegisterReadWriteTest::setup_passes(), input, expected);
}

TEST(RegisterReadWrite, ApplyBlockWriteAfterRead) {
    auto input = R"(
        Register<bit<16>, PortId_t>(1024) reg_16b;
        apply {
            ig_md.result_16b = reg_16b.read(ig_intr_md.ingress_port);
            reg_16b.write(ig_intr_md.ingress_port, ig_tm_md.mcast_grp_a);
        })";
    Match::CheckList expected{
        "Register<bit<16>, PortId_t>(32w1024) reg_16b_0;",
        "RegisterAction<bit<16>, PortId_t, bit<16>>(reg_16b_0)",
        "reg_16b_0_`(.*)` = {",
        "void apply(inout bit<16> value, out bit<16> rv) {",
        "bit<16> in_value;",
        "in_value = value;",
        "rv = value;",
        "value = ig_tm_md.mcast_grp_a;",
        "}",
        "};",
        "action `\\1`() {",
        "ig_md.result_16b = reg_16b_0_`\\1`.execute(ig_intr_md.ingress_port);",
        "}",
        "apply {",
        "`\\1`();",
        "}"};
    RUN_CHECK(defs, RegisterReadWriteTest::setup_passes(), input, expected);
}

TEST(RegisterReadWrite, RegisterParamRegisterReadWrite) {
    auto input = R"(
        Register<bit<16>, PortId_t>(1024) reg_16b;
        RegisterParam<bit<16>>(0x1234) reg_param;
        apply {
            reg_16b.write(ig_intr_md.ingress_port, reg_param.read());
        })";
    Match::CheckList expected{"RegisterParam<bit<16>>(16w0x1234) reg_param_`(.*)`;",
                              "Register<bit<16>, PortId_t>(32w1024) reg_16b_0;",
                              "RegisterAction<bit<16>, PortId_t, bit<16>>(reg_16b_0)",
                              "reg_16b_0_`(.*)` = {",
                              "void apply(inout bit<16> value) {",
                              "value = reg_param_`\\1`.read();",
                              "}",
                              "};",
                              "action `\\2`() {",
                              "reg_16b_0_`\\2`.execute(ig_intr_md.ingress_port);",
                              "}",
                              "apply {",
                              "`\\2`();",
                              "}"};
    RUN_CHECK(defs, RegisterReadWriteTest::setup_passes(), input, expected);
}
// Keep definition of RUN_CHECK local for unity builds.
#undef RUN_CHECK

}  // namespace P4::Test
