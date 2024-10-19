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
 * Tests related to container valid coming out of parser
 */

#include <boost/algorithm/string/replace.hpp>

#include "bf-p4c/test/gtest/tofino_gtest_utils.h"
#include "bf_gtest_helpers.h"
#include "gtest/gtest.h"
#include "ir/ir.h"
#include "test/gtest/helpers.h"

namespace P4::Test {

/*
 * Verify that the parser will set containers valid when writing a zero into a
 * header field.
 *
 * Container can be set valid through either of these mechaisms:
 *   - extracting to the container
 *   - adding the container to the init_zero list
 */
TEST(ParserContainerValidTest, Tof1SetHeaderFieldZero) {
    // P4 program
    const char *p4_prog = R"(
header h {
    bit<32> d1;
}

struct headers_t {
    h h1;
}

struct metadata_t {}

parser IngressParser(
        packet_in pkt,
        out headers_t hdr,
        out metadata_t ig_md,
        out ingress_intrinsic_metadata_t ig_intr_md) {

    state start {
        pkt.extract(ig_intr_md);
        pkt.advance(PORT_METADATA_SIZE);
        transition parse_h1;
    }

    state parse_h1 {
        hdr.h1.setValid();
        hdr.h1.d1 = 0;
        transition accept;
    }
}

parser EgressParser(
    packet_in pkt,
    out headers_t hdr,
    out metadata_t meta,
    out egress_intrinsic_metadata_t eg_intr_md) {

    state start {
        transition accept;
    }
}

control Ingress(
    inout headers_t hdr,
    inout metadata_t ig_md,
    in ingress_intrinsic_metadata_t ig_intr_md,
    in ingress_intrinsic_metadata_from_parser_t ig_intr_prsr_md,
    inout ingress_intrinsic_metadata_for_deparser_t ig_intr_dprsr_md,
    inout ingress_intrinsic_metadata_for_tm_t ig_intr_tm_md) {
    apply {
        ig_intr_tm_md.ucast_egress_port = ig_intr_md.ingress_port;
    }
}

control Egress(
    inout headers_t hdr,
    inout metadata_t meta,
    in egress_intrinsic_metadata_t eg_intr_md,
    in egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr,
    inout egress_intrinsic_metadata_for_deparser_t ig_intr_dprs_md,
    inout egress_intrinsic_metadata_for_output_port_t eg_intr_oport_md) {
    apply {
    }
}

control IngressDeparser(
    packet_out pkt,
    inout headers_t hdr,
    in metadata_t ig_md,
    in ingress_intrinsic_metadata_for_deparser_t ig_intr_dprsr_md) {
    apply {
        pkt.emit(hdr);
    }
}

control EgressDeparser(
    packet_out pkt,
    inout headers_t hdr,
    in metadata_t eg_md,
    in egress_intrinsic_metadata_for_deparser_t eg_intr_dprsr_md) {
    apply {
        pkt.emit(hdr);
    }
}

Pipeline(
    IngressParser(),
    Ingress(),
    IngressDeparser(),
    EgressParser(),
    Egress(),
    EgressDeparser()) pipe;

Switch(pipe) main;)";

    auto blk = TestCode(TestCode::Hdr::Tofino1arch, p4_prog);
    blk.flags(Match::TrimWhiteSpace | Match::TrimAnnotations);

    EXPECT_TRUE(blk.CreateBackend());
    EXPECT_TRUE(blk.apply_pass(TestCode::Pass::FullBackend));

    std::cerr << blk.extract_code(TestCode::CodeBlock::PhvAsm) << std::endl;
    std::cerr << blk.extract_code(TestCode::CodeBlock::ParserIAsm) << std::endl;

    // Identify the container assigned to the h1.d1 field
    auto res =
        blk.match(TestCode::CodeBlock::PhvAsm, Match::CheckList{"`.*`", R"(`hdr.h1.d1: (\w+)`)"});
    EXPECT_TRUE(res.success) << " pos=" << res.pos << " count=" << res.count << "\n'"
                             << blk.extract_code(TestCode::CodeBlock::ParserEAsm) << "'\n";
    if (res.success) {
        auto cont = res.matches[1];

        // Verify that the field is either assigned zero or added to the init_zero list
        auto res_assign_zero =
            blk.match(TestCode::CodeBlock::ParserIAsm, Match::CheckList{"`.*`", cont, ": 0"});
        auto res_init_zero =
            blk.match(TestCode::CodeBlock::ParserIAsm,
                      Match::CheckList{"`.*`", "init_zero: [", "`[^\\[]*`", cont, "`[^\\]]*\\]`"});

        EXPECT_TRUE(res_assign_zero.success || res_init_zero.success)
            << " pos=" << res.pos << " count=" << res.count << "\n'"
            << blk.extract_code(TestCode::CodeBlock::ParserIAsm) << "'\n";
    }
}

}  // namespace P4::Test
