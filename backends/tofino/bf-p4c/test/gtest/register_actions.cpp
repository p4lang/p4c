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

#include "bf-p4c/bf-p4c-options.h"
#include "bf-p4c/control-plane/runtime.h"
#include "bf_gtest_helpers.h"
#include "control-plane/p4RuntimeArchStandard.h"
#include "gtest/gtest.h"

namespace P4::Test {

static auto MAIN_P4_CODE = R"(
    control SwitchIngressDeparser(packet_out pkt,
                                    inout header_t hdr,
                                    in metadata_t ig_md,
                                    in ingress_intrinsic_metadata_for_deparser_t ig_intr_dprsr_md) {
            apply {}
        }
        parser TofinoEgressParser(
                packet_in pkt,
                out egress_intrinsic_metadata_t eg_intr_md) {
            state start {}
        }
        parser SwitchEgressParser(packet_in pkt,
                                out header_t hdr,
                                out metadata_t eg_md,
                                out egress_intrinsic_metadata_t eg_intr_md)  {
            state start {}
        }
        control SwitchEgress(inout header_t hdr,
                            inout metadata_t eg_md,
                            in egress_intrinsic_metadata_t eg_intr_md,
                            in egress_intrinsic_metadata_from_parser_t eg_intr_from_prsr,
                            inout egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprsr,
                            inout egress_intrinsic_metadata_for_output_port_t eg_intr_md_for_oport) {
            apply {}
        }
        control SwitchEgressDeparser(packet_out pkt,
                                    inout header_t hdr,
                                    in metadata_t eg_md,
                                    in egress_intrinsic_metadata_for_deparser_t eg_intr_dprsr_md) {
            apply {}
        }

        Pipeline(SwitchIngressParser(), SwitchIngress(), SwitchIngressDeparser(), SwitchEgressParser(), SwitchEgress(), SwitchEgressDeparser()) pipe;
        Switch(pipe) main;
)";

TEST(RegisterAction, SignedNegative) {
    // Testing correct parsing of negative int values
    auto source = R"(
        const int<32> a1 = -42;
        typedef int<16> index_t;
        struct metadata_t {
            index_t index;
        }
        header ethernet_h {
            int<16> ether_type;
        }
        struct header_t {
            int<32> src_addr;
        }
        parser SwitchIngressParser(packet_in pkt,
                                out header_t hdr,
                                out metadata_t ig_md,
                                out ingress_intrinsic_metadata_t ig_intr_md)  {
            state start {}
        }
        control SwitchIngress(inout header_t hdr,
                            inout metadata_t ig_md,
                            in ingress_intrinsic_metadata_t ig_intr_md,
                            in ingress_intrinsic_metadata_from_parser_t ig_intr_prsr_md,
                            inout ingress_intrinsic_metadata_for_deparser_t ig_intr_dprsr_md,
                            inout ingress_intrinsic_metadata_for_tm_t ig_intr_tm_md) {
            Register<int<32>, index_t>(65536, a1) reg_test_1;
            RegisterAction<int<32>, index_t, int<32>>(reg_test_1) regact_test_1 = {
                void apply(inout int<32> value, out int<32> rv) { 
                    value = value + 1;
                    rv = value;
                }
            };
            action act_test_1() {
                hdr.src_addr = regact_test_1.execute(ig_md.index);
            }
            apply {
                act_test_1();
            }
        }
    )";

    auto testCode = TestCode(TestCode::Hdr::Tofino2arch, std::string(source) + MAIN_P4_CODE);
    EXPECT_TRUE(testCode.CreateBackend());
    EXPECT_TRUE(testCode.apply_pass(TestCode::Pass::FullBackend));

    Match::CheckList expected{"`.*`", "initial_value: { lo: -42 }"};
    auto res = testCode.match(TestCode::CodeBlock::MauAsm, expected);
    EXPECT_TRUE(res.success) << "Mismatch in the generated assembly. " << "pos=" << res.pos
                             << ", count=" << res.count << "\n"
                             << testCode.extract_code(TestCode::CodeBlock::MauAsm) << "\n";
}

TEST(RegisterAction, UnsignedBitInt) {
    // Testing correct parsing of unsigned bigger than (2^31)-1
    auto source = R"(
        const bit<32> a1 = 0x8000_0000;
        typedef bit<16> index_t;
        struct metadata_t {
            index_t index;
        }
        header ethernet_h {
            int<16> ether_type;
        }
        struct header_t {
            bit<32> src_addr;
        }
        parser SwitchIngressParser(packet_in pkt,
                                out header_t hdr,
                                out metadata_t ig_md,
                                out ingress_intrinsic_metadata_t ig_intr_md)  {
            state start {}
        }
        control SwitchIngress(inout header_t hdr,
                            inout metadata_t ig_md,
                            in ingress_intrinsic_metadata_t ig_intr_md,
                            in ingress_intrinsic_metadata_from_parser_t ig_intr_prsr_md,
                            inout ingress_intrinsic_metadata_for_deparser_t ig_intr_dprsr_md,
                            inout ingress_intrinsic_metadata_for_tm_t ig_intr_tm_md) {
            Register<bit<32>, index_t>(65536, a1) reg_test_1;
            RegisterAction<bit<32>, index_t, bit<32>>(reg_test_1) regact_test_1 = {
                void apply(inout bit<32> value, out bit<32> rv) { 
                    value = value + 1;
                    rv = value;
                }
            };
            action act_test_1() {
                hdr.src_addr = regact_test_1.execute(ig_md.index);
            }
            apply {
                act_test_1();
            }
        }
    )";

    auto testCode = TestCode(TestCode::Hdr::Tofino2arch, std::string(source) + MAIN_P4_CODE);
    EXPECT_TRUE(testCode.CreateBackend());
    EXPECT_TRUE(testCode.apply_pass(TestCode::Pass::FullBackend));

    Match::CheckList expected{"`.*`", "initial_value: { lo: 2147483648 }"};
    auto res = testCode.match(TestCode::CodeBlock::MauAsm, expected);
    EXPECT_TRUE(res.success) << "Mismatch in the generated assembly. " << "pos=" << res.pos
                             << ", count=" << res.count << "\n"
                             << testCode.extract_code(TestCode::CodeBlock::MauAsm) << "\n";
}

}  // namespace P4::Test
