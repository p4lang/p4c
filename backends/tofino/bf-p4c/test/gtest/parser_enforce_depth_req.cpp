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
 * Verify that parser pad requirements are enforced correctly.
 *
 * Extra states should be added to reach minimum.
 * Error should be generated when exceeding the maximum depth
 */

#include <boost/algorithm/string/replace.hpp>

#include "bf-p4c/test/gtest/tofino_gtest_utils.h"
#include "bf_gtest_helpers.h"
#include "gtest/gtest.h"
#include "ir/ir.h"
#include "test/gtest/helpers.h"

namespace P4::Test {

using namespace Match;  // To remove noise & reduce line lengths.

// P4 program shell for all tests
std::string prog_shell() {
    return R"(
    header ethernet_t {
        bit<48> dst_addr;
        bit<48> src_addr;
        bit<16> ethertype;
    }

    header hdr32b_t {
        bit<32> f1;
        bit<32> f2;
        bit<32> f3;
        bit<32> f4;
        bit<32> f5;
        bit<32> f6;
        bit<32> f7;
        bit<32> f8;
    };

    struct metadata_t {}

    %0% // Define struct headers_t{};

    parser ingress_parser(packet_in pkt, out headers_t hdr,
                          out metadata_t ig_md, out ingress_intrinsic_metadata_t ig_intr_md)
        {state start {transition accept;}}

    control ingress_control(inout headers_t hdr, inout metadata_t ig_md,
                            in ingress_intrinsic_metadata_t ig_intr_md,
                            in ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
                            inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
                            inout ingress_intrinsic_metadata_for_tm_t ig_tm_md)
        {apply{}}

    control ingress_deparser(packet_out pkt, inout headers_t hdr, in metadata_t ig_md,
                             in ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md)
        {apply{}}

    parser egress_parser(packet_in pkt, out headers_t hdr, out metadata_t eg_md,
                         out egress_intrinsic_metadata_t eg_intr_md) {

        state start {
            pkt.extract(eg_intr_md);
            transition parse_ethernet;
        }

        state parse_ethernet {
            pkt.extract(hdr.eth);
            transition select(hdr.eth.ethertype) {
                0xcdef : parse_h1;
                default : accept;
            }
        }

        %1%
    }

    control egress_control(inout headers_t hdr, inout metadata_t eg_md,
                           in egress_intrinsic_metadata_t eg_intr_md,
                           in egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr,
                           inout egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprsr,
                           inout egress_intrinsic_metadata_for_output_port_t eg_intr_md_for_oport)
        {%2%}

    control egress_deparser(packet_out pkt, inout headers_t hdr, in metadata_t eg_md,
                            in egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprsr)
        { apply { pkt.emit(hdr); } }

    Pipeline(ingress_parser(),
             ingress_control(),
             ingress_deparser(),
             egress_parser(),
             egress_control(),
             egress_deparser()) pipeline;
    Switch(pipeline) main;)";
}

/** Verify that pad states are inserted for a program that doesn't exceed max.
 */
TEST(ParserEnforceDepthReqTest, PadStates) {
    std::string hdrs = R"(
        struct headers_t {
            ethernet_t eth;
            hdr32b_t h1;
            hdr32b_t h2;
            hdr32b_t h3;
        }
    )";

    std::string prsr = R"(
        state parse_h1 {
            pkt.extract(hdr.h1);
            transition select(hdr.h1.f1[31:24]) {
                0xf : parse_h2;
                0xe : reject;
                default : accept;
            }
        }

        state parse_h2 {
            pkt.extract(hdr.h2);
            transition select(hdr.h2.f1[31:24]) {
                0xf : parse_h3;
                0xe : reject;
                default : accept;
            }
        }

        state parse_h3 {
            pkt.extract(hdr.h3);
            transition accept;
        }
    )";

    std::string ctrl = R"(
        apply {
            // Operation to ensure that the h3 header is extracted
            if (hdr.h3.isValid() && hdr.h3.f6[1:1] != 0x0) {
                hdr.h3.f7 = hdr.h3.f8;
            }
        }
    )";

    auto blk = TestCode(TestCode::Hdr::Tofino1arch, prog_shell(), {hdrs, prsr, ctrl});
    blk.flags(TrimWhiteSpace | TrimAnnotations);

    // TestCode disables min depth limits by default. Re-enable.
    BackendOptions().disable_parse_min_depth_limit = false;

    EXPECT_TRUE(blk.CreateBackend());
    EXPECT_TRUE(blk.apply_pass(TestCode::Pass::FullBackend));

    // Check that the min_parse_depth_access states are present in the output file.
    auto res = blk.match(
        TestCode::CodeBlock::ParserEAsm,
        CheckList{"parser egress: start", "`.*`", "min_parse_depth_accept_initial:", "`.*`",
                  "min_parse_depth_accept_loop.$split_0:", "`.*`",
                  "min_parse_depth_accept_loop.$it1.$split_0:"});
    EXPECT_TRUE(res.success) << " pos=" << res.pos << " count=" << res.count << "\n'"
                             << blk.extract_code(TestCode::CodeBlock::ParserEAsm) << "'\n";

    res =
        blk.match(TestCode::CodeBlock::ParserEAsm,
                  CheckList{"parser egress: start", "`.*`", "next: min_parse_depth_accept_initial",
                            "`.*`", "next: min_parse_depth_accept_loop.$split_0", "`.*`",
                            "next: min_parse_depth_accept_loop.$it1.$split_0"});
    EXPECT_TRUE(res.success) << " pos=" << res.pos << " count=" << res.count << "\n'"
                             << blk.extract_code(TestCode::CodeBlock::ParserEAsm) << "'\n";

    // Check that the following states are not present anymore, since they
    // should have been merged.
    res = blk.match(TestCode::CodeBlock::ParserEAsm,
                    CheckList{"parser egress: start", "`.*`", "min_parse_depth_accept_loop:"});
    EXPECT_FALSE(res.success) << " pos=" << res.pos << " count=" << res.count << "\n'"
                              << blk.extract_code(TestCode::CodeBlock::ParserEAsm) << "'\n";

    res = blk.match(TestCode::CodeBlock::ParserEAsm,
                    CheckList{"parser egress: start", "`.*`", "min_parse_depth_accept_loop.$it1:"});
    EXPECT_FALSE(res.success) << " pos=" << res.pos << " count=" << res.count << "\n'"
                              << blk.extract_code(TestCode::CodeBlock::ParserEAsm) << "'\n";

    res = blk.match(TestCode::CodeBlock::ParserEAsm,
                    CheckList{"parser egress: start", "`.*`", "min_parse_depth_accept_loop.$it2:"});
    EXPECT_FALSE(res.success) << " pos=" << res.pos << " count=" << res.count << "\n'"
                              << blk.extract_code(TestCode::CodeBlock::ParserEAsm) << "'\n";

    res = blk.match(
        TestCode::CodeBlock::ParserEAsm,
        CheckList{"parser egress: start", "`.*`", "min_parse_depth_accept_loop.$it2.$split_0:"});
    EXPECT_FALSE(res.success) << " pos=" << res.pos << " count=" << res.count << "\n'"
                              << blk.extract_code(TestCode::CodeBlock::ParserEAsm) << "'\n";
}

/** Verify that pad states are inserted for a program that doesn't exceed max.
 */
TEST(ParserEnforceDepthReqTest, MaxDepthViolated) {
    std::string hdrs = R"(
        struct headers_t {
            ethernet_t eth;
            hdr32b_t h1;
            hdr32b_t h2;
            hdr32b_t h3;
            hdr32b_t h4;
        }
    )";

    std::string prsr = R"(
        state parse_h1 {
            pkt.extract(hdr.h1);
            transition select(hdr.h1.f1[31:24]) {
                0xf : parse_h2;
                0xe : reject;
                default : accept;
            }
        }

        state parse_h2 {
            pkt.extract(hdr.h2);
            transition select(hdr.h2.f1[31:24]) {
                0xf : parse_h3;
                0xe : reject;
                default : accept;
            }
        }

        state parse_h3 {
            pkt.extract(hdr.h3);
            transition select(hdr.h2.f1[31:24]) {
                0xf : parse_h4;
                0xe : reject;
                default : accept;
            }
        }

        state parse_h4 {
            pkt.extract(hdr.h4);
            transition accept;
        }
    )";

    std::string ctrl = R"(
        apply {
            // Operation to ensure that the h4 header is extracted
            if (hdr.h4.isValid() && hdr.h4.f6[1:1] != 0x0) {
                hdr.h4.f7 = hdr.h4.f8;
            }
        }
    )";

    auto blk = TestCode(TestCode::Hdr::Tofino1arch, prog_shell(), {hdrs, prsr, ctrl});
    blk.flags(TrimWhiteSpace | TrimAnnotations);

    testing::internal::CaptureStderr();
    EXPECT_FALSE(blk.CreateBackend());
    auto stderr = testing::internal::GetCapturedStderr();
    EXPECT_TRUE(stderr.find("longest path through parser") != std::string::npos &&
                stderr.find("exceeds maximum parse depth") != std::string::npos);
}

}  // namespace P4::Test
