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

#include <optional>

#include "bf-p4c/test/gtest/tofino_gtest_utils.h"
#include "bf_gtest_helpers.h"
#include "gtest/gtest.h"
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
