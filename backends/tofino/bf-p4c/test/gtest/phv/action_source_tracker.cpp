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
#include "bf-p4c/phv/action_source_tracker.h"
#include "bf-p4c/mau/instruction_selection.h"

namespace P4::Test {

namespace ActionSourceTrackerTestNs {

std::optional<TofinoPipeTestCase>
createActionTest(const std::string& ingressPipeline,
                 const std::string& egressPipeline) {
    auto source = P4_SOURCE(P4Headers::V1MODEL, R"(
        header H { bit<8> f1; bit<16> f2; bit<24> f3; bit<32> f4; bit<7> f5; bit<9> f6; bit<32> f7; }
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

class ActionSourceTrackerTest : public TofinoBackendTest { };

TEST_F(ActionSourceTrackerTest, basic) {
    auto test = ActionSourceTrackerTestNs::createActionTest(P4_SOURCE(P4Headers::NONE, R"(
    action a1(bit<8> f1_src) {
        headers.h1.f1 = f1_src;
        headers.h2.f1 = f1_src;
    }
    action a2() {
        headers.h1.f2[7:0] = headers.h1.f1;
        headers.h1.f2[15:8] = headers.h2.f1;
    }
    action a3() {
        headers.h1.f3 = headers.h2.f3 ^ 5;
    }
    action a4() {
        headers.h1.f4 = headers.h1.f4 + headers.h2.f4;
    }
    action a5() {
        headers.h1.f5 = (bit<7>)headers.h2.f6;
    }
    action a6() {
        headers.h1.f7[16:1] = headers.h2.f7[16:1];
        headers.h1.f7[25:17] = 0;
    }
    action a7() {
        headers.h1.f7[20:3] = headers.h2.f7[20:3];
        headers.h1.f7[31:28] = 0;
    }
    table test1 {
        key = {
            headers.h1.f1 : exact;
        }
        actions = {
            a1; a2; a3; a4; a5; a6; a7;
        }
        size = 1024;
    }
    apply {
        test1.apply();
    }
    )"), P4_SOURCE(P4Headers::NONE, R"(
        apply { }
    )"));

    const std::string golden =
        R"(ingress::headers.h1.f1 [0:7] { ingress.a1: (move AD_OR_CONST) }
ingress::headers.h2.f1 [0:7] { ingress.a1: (move AD_OR_CONST) }
ingress::headers.h1.f2 [0:7] { ingress.a2: (move ingress::headers.h1.f1<8> ^0 ^bit[0..7] deparsed exact_containers [0:7]) }
ingress::headers.h1.f2 [8:15] { ingress.a2: (move ingress::headers.h2.f1<8> ^0 ^bit[0..135] deparsed exact_containers [0:7]) }
ingress::headers.h1.f3 [0:23] { ingress.a3: (bitwise ingress::headers.h2.f3<24> ^0 ^bit[0..175] deparsed exact_containers [0:23]), (bitwise AD_OR_CONST) }
ingress::headers.h1.f4 [0:31] { ingress.a4: (whole_container ingress::headers.h1.f4<32> ^0 ^bit[0..79] deparsed exact_containers [0:31]), (whole_container ingress::headers.h2.f4<32> ^0 ^bit[0..207] deparsed exact_containers [0:31]) }
ingress::headers.h1.f5 [0:6] { ingress.a5: (move ingress::headers.h2.f6<9> ^0 ^bit[0..223] deparsed exact_containers [0:6]) }
ingress::headers.h1.f7 [1:2] { ingress.a6: (move ingress::headers.h2.f7<32> ^1 ^bit[0..254] deparsed exact_containers [1:2]) }
ingress::headers.h1.f7 [3:16] { ingress.a6: (move ingress::headers.h2.f7<32> ^3 ^bit[0..252] deparsed exact_containers [3:16]) }{ ingress.a7: (move ingress::headers.h2.f7<32> ^3 ^bit[0..252] deparsed exact_containers [3:16]) }
ingress::headers.h1.f7 [17:20] { ingress.a6: (move AD_OR_CONST) }{ ingress.a7: (move ingress::headers.h2.f7<32> ^1 ^bit[0..238] deparsed exact_containers [17:20]) }
ingress::headers.h1.f7 [21:25] { ingress.a6: (move AD_OR_CONST) }
ingress::headers.h1.f7 [28:31] { ingress.a7: (move AD_OR_CONST) }
)";

    ASSERT_TRUE(test);
    PhvInfo phv;
    ReductionOrInfo red_info;
    auto *post_pm_pipe = ActionSourceTrackerTestNs::runInitialPassManager(test->pipe, &phv);
    PHV::ActionSourceTracker tracker(phv, red_info);
    post_pm_pipe = post_pm_pipe->apply(tracker);
    std::stringstream ss;
    ss << tracker;
    EXPECT_EQ(golden, ss.str());
}

}  // namespace P4::Test
