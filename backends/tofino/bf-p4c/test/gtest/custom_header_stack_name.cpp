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
 * This test tests whether the intrinsic metadata identifiers on bf-runtime file
 * are correct for different pipes.
 */

#include <string>
#include <fstream>
#include <streambuf>

#include "bf_gtest_helpers.h"
#include "gtest/gtest.h"
#include "bf-p4c/bf-p4c-options.h"
#include "bf-p4c/control-plane/runtime.h"
#include "control-plane/p4RuntimeArchStandard.h"
#include "control-plane/p4RuntimeSerializer.h"

namespace P4::Test {

TEST(CustomHeaderStackName, Test1) {
    // Simple program with 4 pipes with different parsers
    auto source = R"(
        header vlan_t {
            bit<3> pri;
            bit<1> cfi;
            bit<12> id;
            bit<16> type;
        }
        struct headers {
            vlan_t[2] vlan;
        }
        struct metadata { }

        parser iparser(
         packet_in pkt,
         out headers hdr,
         out metadata meta,
         out ingress_intrinsic_metadata_t ing_meta) {
            state start {
                pkt.extract(ing_meta);
                pkt.extract(hdr.vlan[0]);
                pkt.extract(hdr.vlan[1]);
                transition accept;
            }
        }

        control ingress(
         inout headers hdr,
         inout metadata meta,
         in ingress_intrinsic_metadata_t ing_meta,
         in ingress_intrinsic_metadata_from_parser_t ing_prsr_meta,
         inout ingress_intrinsic_metadata_for_deparser_t ing_dprsr_meta,
         inout ingress_intrinsic_metadata_for_tm_t ing_tm_meta) {
            action a1() {
                hdr.vlan[0].id = 0;
            }
            table t1 {
                key = {
                    hdr.vlan[0].isValid() : exact @name("vlan[0]");
                    hdr.vlan[0].id[10:1] : exact @name("vlan[0].id");
                }
                actions = {
                    a1;
                }
            }
            apply {
                t1.apply();
            }
        }

        control ideparser(
         packet_out pkt,
         inout headers hdr,
         in metadata meta,
         in ingress_intrinsic_metadata_for_deparser_t ing_dprsr_meta) {
            apply {
                pkt.emit(hdr);
            }
        }

        parser eparser(
         packet_in pkt,
         out headers hdr,
         out metadata meta,
         out egress_intrinsic_metadata_t eg_meta) {
            state start {
                pkt.extract(eg_meta);
                transition accept;
            }
        }

        control egress(
         inout headers hdr,
         inout metadata meta,
         in egress_intrinsic_metadata_t eg_meta,
         in egress_intrinsic_metadata_from_parser_t eg_prsr_meta,
         inout egress_intrinsic_metadata_for_deparser_t eg_dprsr_meta,
         inout egress_intrinsic_metadata_for_output_port_t eg_oprt_meta) {
             apply { }
        }

        control edeparser(
         packet_out pkt,
         inout headers hdr,
         in metadata meta,
         in egress_intrinsic_metadata_for_deparser_t eg_dprsr_meta) {
            apply {
                pkt.emit(hdr);
            }
        }
    
        Pipeline(
            iparser(),
            ingress(),
            ideparser(),
            eparser(),
            egress(),
            edeparser()
        ) pipe;
        Switch(pipe) main;
    )";

    std::string output_dir = "CustomHeaderStackName";
    std::string bfrt_file = output_dir + "/bf-rt.json";

    // Create a program
    auto testCode = TestCode(TestCode::Hdr::Tofino2arch, source,
                        {}, "",
                        {"-o", output_dir,
                         "--bf-rt-schema", bfrt_file});
    // Run frontend
    EXPECT_TRUE(testCode.apply_pass(TestCode::Pass::FullFrontend));

    // Generate runtime information
    auto& options = BackendOptions();
    BFN::generateRuntime(testCode.get_program(), options);
    // BFN's generateRuntime re-registers handler for psa architecture to a BFN handler
    // Re-register the original p4c one so that (plain) p4c tests are not affected
    auto p4RuntimeSerializer = P4::P4RuntimeSerializer::get();
    p4RuntimeSerializer->registerArch(
        "psa"_cs,
        new P4::ControlPlaneAPI::Standard::PSAArchHandlerBuilder());
    // Check the runtime JSON file
    std::ifstream bfrt_stream(bfrt_file);
    EXPECT_TRUE(bfrt_stream);
    std::string line;
    bool found_bracketed_name = false;
    while (std::getline(bfrt_stream, line)) {
        // There should be no vlan[
        found_bracketed_name = (line.find("vlan[") != std::string::npos);
        EXPECT_FALSE(found_bracketed_name);
    }
}

}  // namespace P4::Test
