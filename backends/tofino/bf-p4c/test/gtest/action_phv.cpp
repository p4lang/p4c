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

#include <optional>
#include <type_traits>
#include <boost/algorithm/string/replace.hpp>
#include "gtest/gtest.h"

#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/error.h"
#include "test/gtest/helpers.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/common/header_stack.h"
#include "bf-p4c/test/gtest/tofino_gtest_utils.h"
#include "bf-p4c/phv/action_phv_constraints.h"
#include "bf-p4c/mau/instruction_selection.h"

namespace P4::Test {

namespace {

std::optional<TofinoPipeTestCase>
createActionTest(const std::string& ingressPipeline,
                 const std::string& egressPipeline) {
    auto source = P4_SOURCE(P4Headers::V1MODEL, R"(
        header H { bit<8> field1; bit<8> field2; bit<8> field3; bit<8> field4; }
        struct Headers { H h1; H h2; }
        struct Metadata { H h; }

        parser parse(packet_in packet, out Headers headers, inout Metadata meta,
                 inout standard_metadata_t sm) {
            state start {
                packet.extract(headers.h1);
                packet.extract(headers.h2);
                transition accept;
            }
        }

        control verifyChecksum(inout Headers headers, inout Metadata meta) { apply { } }
        control ingress(inout Headers headers, inout Metadata meta,
                        inout standard_metadata_t sm) {
%INGRESS_PIPELINE%
        }

        control egress(inout Headers headers, inout Metadata meta,
                       inout standard_metadata_t sm) {
%EGRESS_PIPELINE%
        }

        control computeChecksum(inout Headers headers, inout Metadata meta) {
            apply { } }

        control deparse(packet_out packet, in Headers headers) {
            apply {
                packet.emit(headers.h1);
                packet.emit(headers.h2);
            }
        }

        V1Switch(parse(), verifyChecksum(), ingress(), egress(),
                 computeChecksum(), deparse()) main;
    )");

    boost::replace_first(source, "%INGRESS_PIPELINE%", ingressPipeline);
    boost::replace_first(source, "%EGRESS_PIPELINE%", egressPipeline);

    auto& options = BackendOptions();
    options.langVersion = CompilerOptions::FrontendVersion::P4_16;
    options.target = "tofino"_cs;
    options.arch = "v1model"_cs;
    options.disable_parse_min_depth_limit = true;

    return TofinoPipeTestCase::createWithThreadLocalInstances(source);
}

const IR::BFN::Pipe *runInitialPassManager(const IR::BFN::Pipe* pipe, PhvInfo *phv) {
    PassManager quick_backend = {
        new CollectHeaderStackInfo,
        new CollectPhvInfo(*phv),
        new DoInstructionSelection(*phv)
    };

    return pipe->apply(quick_backend);
}

}  // namespace

class ActionPhv : public TofinoBackendTest { };

TEST_F(ActionPhv, IngressSingleTableSharedWrites) {
    auto test = createActionTest(P4_SOURCE(P4Headers::NONE, R"(
        action first(bit<8> param1, bit<8> param2) {
            headers.h1.field1 = param1;
            headers.h1.field2 = param2;
        }

        action second(bit<8> param1, bit<8> param2, bit<8> param3) {
            headers.h1.field1 = param1;
            headers.h1.field3 = param2;
            headers.h1.field4 = param3;
        }

        table test1 {
            key = { headers.h1.field1 : ternary; }
            actions = {
                first;
                second;
            }
        }

        apply {
            test1.apply();
        }

    )"), P4_SOURCE(P4Headers::NONE, R"(
        apply { }
    )"));

    ASSERT_TRUE(test);
    PhvInfo phv;
    auto *post_pm_pipe = runInitialPassManager(test->pipe, &phv);
    ActionPhvConstraints apc(phv);
    post_pm_pipe = post_pm_pipe->apply(apc);

    std::vector<const IR::MAU::Action *> actionList;
    forAllMatching<IR::MAU::Action>(post_pm_pipe, [&](const IR::MAU::Action* act) {
            actionList.push_back(act);
            });

    EXPECT_TRUE(apc.is_in_field_writes_to_actions("ingress::h1.field1", actionList[0]));
    EXPECT_TRUE(apc.is_in_field_writes_to_actions("ingress::h1.field1", actionList[1]));
    EXPECT_FALSE(apc.is_in_field_writes_to_actions("ingress::h1.field1", actionList[2]));
    EXPECT_TRUE(apc.is_in_field_writes_to_actions("ingress::h1.field2", actionList[0]));
    EXPECT_FALSE(apc.is_in_field_writes_to_actions("ingress::h1.field2", actionList[1]));
    EXPECT_FALSE(apc.is_in_field_writes_to_actions("ingress::h1.field2", actionList[2]));
    EXPECT_FALSE(apc.is_in_field_writes_to_actions("ingress::h1.field3", actionList[0]));
    EXPECT_TRUE(apc.is_in_field_writes_to_actions("ingress::h1.field3", actionList[1]));
    EXPECT_FALSE(apc.is_in_field_writes_to_actions("ingress::h1.field3", actionList[2]));
    EXPECT_FALSE(apc.is_in_field_writes_to_actions("ingress::h1.field4", actionList[0]));
    EXPECT_TRUE(apc.is_in_field_writes_to_actions("ingress::h1.field4", actionList[1]));
    EXPECT_FALSE(apc.is_in_field_writes_to_actions("ingress::h1.field4", actionList[2]));

    EXPECT_TRUE(apc.is_in_action_to_writes(actionList[0], "ingress::h1.field1"));
    EXPECT_TRUE(apc.is_in_action_to_writes(actionList[1], "ingress::h1.field1"));
    EXPECT_FALSE(apc.is_in_action_to_writes(actionList[2], "ingress::h1.field1"));
    EXPECT_TRUE(apc.is_in_action_to_writes(actionList[0], "ingress::h1.field2"));
    EXPECT_FALSE(apc.is_in_action_to_writes(actionList[1], "ingress::h1.field2"));
    EXPECT_FALSE(apc.is_in_action_to_writes(actionList[2], "ingress::h1.field2"));
    EXPECT_FALSE(apc.is_in_action_to_writes(actionList[0], "ingress::h1.field3"));
    EXPECT_TRUE(apc.is_in_action_to_writes(actionList[1], "ingress::h1.field3"));
    EXPECT_FALSE(apc.is_in_action_to_writes(actionList[2], "ingress::h1.field3"));
    EXPECT_FALSE(apc.is_in_action_to_writes(actionList[0], "ingress::h1.field4"));
    EXPECT_TRUE(apc.is_in_action_to_writes(actionList[1], "ingress::h1.field4"));
    EXPECT_FALSE(apc.is_in_action_to_writes(actionList[2], "ingress::h1.field4"));
}

TEST_F(ActionPhv, IngressMultipleTablesSharedWrites) {
    auto test = createActionTest(P4_SOURCE(P4Headers::NONE, R"(
        action first(bit<8> param1, bit<8> param2) {
            headers.h1.field1 = param1;
            headers.h1.field4 = param2;
        }

        action second(bit<8> param1, bit<8> param2, bit<8> param3) {
            headers.h1.field1 = param1;
            headers.h2.field3 = param2;
            headers.h2.field4 = param3;

        }

        action third(bit<8> param1, bit<8> param2, bit<8> param3) {
            headers.h1.field4 = param1;
            headers.h2.field2 = param2;
            headers.h2.field3 = param3;
        }

        table test1 {
            key = { headers.h1.field1 : ternary; }
            actions = {
                first;
            }
        }

        table test2 {
            key = { headers.h2.field1 : ternary; }
            actions = {
                second;
                third;
            }
        }

        apply {
            test1.apply();
            test2.apply();
        }

    )"), P4_SOURCE(P4Headers::NONE, R"(
        apply { }
    )"));

    ASSERT_TRUE(test);
    PhvInfo phv;
    auto *post_pm_pipe = runInitialPassManager(test->pipe, &phv);
    ActionPhvConstraints apc(phv);
    post_pm_pipe = post_pm_pipe->apply(apc);

    std::vector<const IR::MAU::Action *> actionList;
    forAllMatching<IR::MAU::Action>(post_pm_pipe, [&](const IR::MAU::Action* act) {
            actionList.push_back(act);
            });

    EXPECT_TRUE(apc.is_in_field_writes_to_actions("ingress::h1.field1", actionList[0]));
    EXPECT_TRUE(apc.is_in_field_writes_to_actions("ingress::h1.field1", actionList[2]));
    EXPECT_FALSE(apc.is_in_field_writes_to_actions("ingress::h1.field1", actionList[1]));
    EXPECT_TRUE(apc.is_in_field_writes_to_actions("ingress::h2.field2", actionList[3]));
    EXPECT_FALSE(apc.is_in_field_writes_to_actions("ingress::h2.field2", actionList[1]));
    EXPECT_FALSE(apc.is_in_field_writes_to_actions("ingress::h2.field2", actionList[0]));
    EXPECT_TRUE(apc.is_in_field_writes_to_actions("ingress::h2.field3", actionList[2]));
    EXPECT_TRUE(apc.is_in_field_writes_to_actions("ingress::h2.field3", actionList[3]));
    EXPECT_FALSE(apc.is_in_field_writes_to_actions("ingress::h2.field3", actionList[4]));
    EXPECT_TRUE(apc.is_in_field_writes_to_actions("ingress::h1.field4", actionList[0]));
    EXPECT_TRUE(apc.is_in_field_writes_to_actions("ingress::h2.field4", actionList[2]));
    EXPECT_TRUE(apc.is_in_field_writes_to_actions("ingress::h1.field4", actionList[3]));

    EXPECT_TRUE(apc.is_in_action_to_writes(actionList[0], "ingress::h1.field1"));
    EXPECT_TRUE(apc.is_in_action_to_writes(actionList[0], "ingress::h1.field4"));
    EXPECT_TRUE(apc.is_in_action_to_writes(actionList[2], "ingress::h1.field1"));
    EXPECT_TRUE(apc.is_in_action_to_writes(actionList[2], "ingress::h2.field3"));
    EXPECT_TRUE(apc.is_in_action_to_writes(actionList[2], "ingress::h2.field4"));
    EXPECT_TRUE(apc.is_in_action_to_writes(actionList[3], "ingress::h1.field4"));
    EXPECT_TRUE(apc.is_in_action_to_writes(actionList[3], "ingress::h2.field2"));
    EXPECT_TRUE(apc.is_in_action_to_writes(actionList[3], "ingress::h2.field3"));
    EXPECT_FALSE(apc.is_in_action_to_writes(actionList[1], "ingress::h1.field1"));
    EXPECT_FALSE(apc.is_in_action_to_writes(actionList[4], "ingress::h2.field4"));
    EXPECT_FALSE(apc.is_in_action_to_writes(actionList[2], "ingress::h2.field2"));
}

TEST_F(ActionPhv, IngressMultipleTablesReads) {
    auto test = createActionTest(P4_SOURCE(P4Headers::NONE, R"(
        action first() {
            headers.h1.field1 = headers.h2.field1;
            headers.h1.field4 = headers.h2.field2;
        }

        action second() {
            headers.h1.field1 = headers.h2.field1;
            headers.h2.field3 = headers.h2.field4;
            headers.h2.field4 = headers.h2.field1;
        }

        action third(bit<8> param) {
            headers.h1.field4 = headers.h2.field4;
            headers.h2.field2 = headers.h1.field2;
            headers.h2.field3 = headers.h1.field2;
            headers.h1.field1 = param;
        }

        table test1 {
            key = { headers.h1.field1 : ternary; }
            actions = {
                first;
            }
        }

        table test2 {
            key = { headers.h2.field1 : ternary; }
            actions = {
                second;
                third;
            }
        }

        apply {
            test1.apply();
            test2.apply();
        }

    )"), P4_SOURCE(P4Headers::NONE, R"(
        apply { }
    )"));

    ASSERT_TRUE(test);
    PhvInfo phv;
    auto *post_pm_pipe = runInitialPassManager(test->pipe, &phv);
    ActionPhvConstraints apc(phv);
    post_pm_pipe = post_pm_pipe->apply(apc);

    std::vector<const IR::MAU::Action *> actionList;
    forAllMatching<IR::MAU::Action>(post_pm_pipe, [&](const IR::MAU::Action* act) {
            actionList.push_back(act);
            });

    EXPECT_TRUE(apc.is_in_write_to_reads(
                    "ingress::h1.field1", actionList[2], "ingress::h2.field1"));
    EXPECT_FALSE(apc.is_in_write_to_reads(
                    "ingress::h1.field1", actionList[0], "ingress::h2.field2"));
    EXPECT_TRUE(apc.is_in_write_to_reads(
                    "ingress::h2.field2", actionList[3], "ingress::h1.field2"));
    // EXPECT_FALSE(apc.is_in_write_to_reads(
    //               "ingress::h1.field1", actionList[3], "ingress::h2.field1"));
}
}  // namespace P4::Test
