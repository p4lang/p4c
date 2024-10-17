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

#include "gtest/gtest.h"

#include "bf_gtest_helpers.h"
#include "bf-p4c/test/gtest/tofino_gtest_utils.h"
#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/error.h"
#include "test/gtest/helpers.h"

namespace P4::Test {

class MauIndirectExternsSingleActionTest : public TofinoBackendTest {};

TEST_F(MauIndirectExternsSingleActionTest, Basic) {
    auto program = R"(
    struct metadata {
        bit<32> value1;
        bit<32> value2;
    }

    header data_t {
        bit<32> f1;
        bit<32> f2;
        bit<16> h1;
        bit<8>  b1;
        bit<8>  b2;
    }

    struct headers {
        data_t data;
    }

    parser ingressParser(packet_in packet, out headers hdr,
                    out metadata meta,
                    out ingress_intrinsic_metadata_t ig_intr_md
    ) {
        state start {
            packet.extract(ig_intr_md);
            packet.advance(PORT_METADATA_SIZE);
            packet.extract(hdr.data);
            transition accept;
        }
    }

    control ingress(inout headers hdr, inout metadata meta,
                    in ingress_intrinsic_metadata_t ig_intr_md,
                    in ingress_intrinsic_metadata_from_parser_t ig_intr_prsr_md,
                    inout ingress_intrinsic_metadata_for_deparser_t ig_intr_dprsr_md,
                    inout ingress_intrinsic_metadata_for_tm_t ig_intr_tm_md,
                    in ghost_intrinsic_metadata_t gmd) {

        Register<bit<32>, bit<9>>(1024, 0) regs1;
        Register<bit<32>, bit<9>>(1024, 0) regs2;

        action test_action_1() {
            meta.value1 = regs1.read(ig_intr_md.ingress_port);
            meta.value2 = regs2.read(ig_intr_md.ingress_port);
        }

        action test_action_2() {
            meta.value1 = meta.value1 + meta.value2;
        }

        table test_table {
            key = {
                ig_intr_md.ingress_port : exact;
            }
            actions = { test_action_1; test_action_2; }
        }

        apply {
            test_table.apply();
            ig_intr_tm_md.ucast_egress_port = (PortId_t)meta.value1;
            ig_intr_tm_md.qid = (QueueId_t)meta.value2;
        }
    }

    control ingressDeparser(packet_out packet, inout headers hdr, in metadata meta,
                            in ingress_intrinsic_metadata_for_deparser_t ig_intr_md_for_dprs) {
        apply {
            packet.emit(hdr);
        }
    }

    parser egressParser(packet_in packet, out headers hdr, out metadata meta,
                        out egress_intrinsic_metadata_t eg_intr_md) {
        state start {
            packet.extract(eg_intr_md);
            transition accept;
        }
    }

    control egress(inout headers hdr, inout metadata meta, in egress_intrinsic_metadata_t eg_intr_md,
                in egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr,
                inout egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprs,
                inout egress_intrinsic_metadata_for_output_port_t eg_intr_md_for_oport) {
        apply {
        }
    }

    control egressDeparser(packet_out packet, inout headers hdr, in metadata meta,
                        in egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprs) {
        apply {
        }
    }

    Pipeline(ingressParser(), ingress(), ingressDeparser(),
            egressParser(), egress(), egressDeparser()) pipe;
    Switch(pipe) main;

    )";

    auto blk = TestCode(TestCode::Hdr::Tofino2arch, program);
    ASSERT_TRUE(blk.apply_pass(TestCode::Pass::FullFrontend));
    ASSERT_TRUE(blk.apply_pass(TestCode::Pass::FullMidend));
    ASSERT_TRUE(blk.apply_pass(TestCode::Pass::ConverterToBackend));
    testing::internal::CaptureStderr();
    EXPECT_FALSE(blk.apply_pass(TestCode::Pass::FullBackend));
    std::string output = testing::internal::GetCapturedStderr();

    auto res = output.find(
        "error: At most one stateful ALU operation with a given address is allowed per action. "
        "Writing to ingress::meta.value2 is not allowed here.");
    ASSERT_NE(res, std::string::npos);
}

}  // namespace P4::Test
