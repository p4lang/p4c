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
 * Verify that parser pad requirements are enforced correctly.
 *
 * Extra states should be added to reach minimum.
 * Error should be generated when exceeding the maximum depth
 */

#include <boost/algorithm/string/replace.hpp>

#include "gtest/gtest.h"
#include "bf_gtest_helpers.h"

#include "ir/ir.h"
#include "bf-p4c/test/gtest/tofino_gtest_utils.h"
#include "test/gtest/helpers.h"

namespace P4::Test {

using namespace Match;  // To remove noise & reduce line lengths.

/** Verify that fields are not packed in with the ingress port.
  * Until a recent fix, checksum validation results were being packed with the
  * ingress port, and these potentially overlap with the version and/or recirc
  * bits.
  */
TEST(ParserExtractReorder, VerifySplitStates) {
    std::string prog = R"(
// ----------------------------------------------------------------------------
// Headers
//-----------------------------------------------------------------------------
header hdr_h {
    bit<8> f1;
    bit<8> f2;
    bit<8> f3;
    bit<8> f4;
}

struct headers_t {
    hdr_h h1;
    hdr_h h2;
    hdr_h h3;
    hdr_h h4;
    hdr_h h5;
    hdr_h h6;
    hdr_h h7;
}

@pa_byte_pack("ingress", "ig_md.f1", "ig_md.f2", "ig_md.f3", "ig_md.f4")
struct metadata_t {
    bit<8> f1;
    bit<8> f2;
    bit<8> f3;
    bit<8> f4;
    bit<8> f5;
    bit<8> f6;
    bit<8> f7;
}

// ---------------------------------------------------------------------------
// Ingress parser
// ---------------------------------------------------------------------------
parser SwitchIngressParser(
        packet_in pkt,
        out headers_t hdr,
        out metadata_t ig_md,
        out ingress_intrinsic_metadata_t ig_intr_md) {

    state start {
        pkt.extract(ig_intr_md);
        pkt.advance(PORT_METADATA_SIZE);
        transition parse_hdr;
    }

    state parse_hdr {
        pkt.extract(hdr.h1);
        pkt.extract(hdr.h2);
        pkt.extract(hdr.h3);
        pkt.extract(hdr.h4);
        pkt.extract(hdr.h5);
        pkt.extract(hdr.h6);
        pkt.extract(hdr.h7);
        hdr_h tmp = pkt.lookahead<hdr_h>();
        //ig_md.f1 = tmp.f4;
        //ig_md.f2 = tmp.f1;
        //ig_md.f4 = tmp.f2;
        //ig_md.f3 = tmp.f3;
        ig_md.f4 = tmp.f4;
        ig_md.f1 = tmp.f1;
        ig_md.f5 = tmp.f2;
        ig_md.f6 = tmp.f4;
        ig_md.f7 = tmp.f3;
        ig_md.f3 = tmp.f3;
        ig_md.f2 = tmp.f2;
        transition accept;
    }
}

// ---------------------------------------------------------------------------
// Ingress Deparser
// ---------------------------------------------------------------------------
control SwitchIngressDeparser(
        packet_out pkt,
        inout headers_t hdr,
        in metadata_t ig_md,
        in ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md) {

    apply {
        pkt.emit(hdr);
    }
}

// ---------------------------------------------------------------------------
// Switch Ingress MAU
// ---------------------------------------------------------------------------
control SwitchIngress(
        inout headers_t hdr,
        inout metadata_t ig_md,
        in ingress_intrinsic_metadata_t ig_intr_md,
        in ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
        inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
        inout ingress_intrinsic_metadata_for_tm_t ig_tm_md) {

    action set_port_h1_a(PortId_t p, bit<8> val) {
        ig_tm_md.ucast_egress_port = p;
        hdr.h1.f2 = val;
    }

    action set_port_h1_b(PortId_t p, bit<8> val) {
        ig_tm_md.ucast_egress_port = p;
        hdr.h1.f4 = val;
    }

    action set_port_h2_a(PortId_t p, bit<8> val) {
        ig_tm_md.ucast_egress_port = p;
        hdr.h2.f2 = val;
    }

    action set_port_h2_b(PortId_t p, bit<8> val) {
        ig_tm_md.ucast_egress_port = p;
        hdr.h2.f4 = val;
    }

    action set_port_h3_a(PortId_t p, bit<8> val) {
        ig_tm_md.ucast_egress_port = p;
        hdr.h3.f2 = val;
    }

    action set_port_h3_b(PortId_t p, bit<8> val) {
        ig_tm_md.ucast_egress_port = p;
        hdr.h3.f4 = val;
    }

    action set_port_h4_a(PortId_t p, bit<8> val) {
        ig_tm_md.ucast_egress_port = p;
        hdr.h4.f2 = val;
    }

    action set_port_h4_b(PortId_t p, bit<8> val) {
        ig_tm_md.ucast_egress_port = p;
        hdr.h4.f4 = val;
    }

    action set_port_h5_a(PortId_t p, bit<8> val) {
        ig_tm_md.ucast_egress_port = p;
        hdr.h5.f2 = val;
    }

    action set_port_h5_b(PortId_t p, bit<8> val) {
        ig_tm_md.ucast_egress_port = p;
        hdr.h5.f4 = val;
    }

    action set_port_h6_a(PortId_t p, bit<8> val) {
        ig_tm_md.ucast_egress_port = p;
        hdr.h6.f2 = val;
    }

    action set_port_h6_b(PortId_t p, bit<8> val) {
        ig_tm_md.ucast_egress_port = p;
        hdr.h6.f4 = val;
    }

    action set_port_h7_a(PortId_t p, bit<8> val) {
        ig_tm_md.ucast_egress_port = p;
        hdr.h7.f2 = val;
    }

    action set_port_h7_b(PortId_t p, bit<8> val) {
        ig_tm_md.ucast_egress_port = p;
        hdr.h7.f4 = val;
    }

    table t {
        key = {
            ig_md.f1: exact;
            ig_md.f2: exact;
            ig_md.f3: exact;
            ig_md.f4: exact;
        }

        actions = {
            set_port_h1_a;
            set_port_h2_a;
            set_port_h3_a;
            set_port_h4_a;
            set_port_h5_a;
            set_port_h6_a;
            set_port_h7_a;

            set_port_h1_b;
            set_port_h2_b;
            set_port_h3_b;
            set_port_h4_b;
            set_port_h5_b;
            set_port_h6_b;
            set_port_h7_b;

            NoAction;
        }

        size = 512;
    }

    apply {
        if (ig_md.f5 != 8w0x0) {
            ig_md.f1 = ig_md.f5;
        } else if (ig_md.f6 != 8w0x0) {
            ig_md.f1 = ig_md.f6;
        } else if (ig_md.f7 != 8w0x0) {
            ig_md.f1 = ig_md.f7;
        }
        t.apply();
    }
}

// ---------------------------------------------------------------------------
// Egress parser
// ---------------------------------------------------------------------------
parser SwitchEgressParser(
        packet_in pkt,
        out headers_t hdr,
        out metadata_t eg_md,
        out egress_intrinsic_metadata_t eg_intr_md) {

    state start {
        pkt.extract(eg_intr_md);
        transition accept;
    }

}

// ---------------------------------------------------------------------------
// Egress Deparser
// ---------------------------------------------------------------------------
control SwitchEgressDeparser(
        packet_out pkt,
        inout headers_t hdr,
        in metadata_t eg_md,
        in egress_intrinsic_metadata_for_deparser_t eg_dprsr_md) {

    apply {}
}

// ---------------------------------------------------------------------------
// Switch Egress MAU
// ---------------------------------------------------------------------------
control SwitchEgress(
        inout headers_t hdr,
        inout metadata_t eg_md,
        in    egress_intrinsic_metadata_t                 eg_intr_md,
        in    egress_intrinsic_metadata_from_parser_t     eg_prsr_md,
        inout egress_intrinsic_metadata_for_deparser_t    eg_dprsr_md,
        inout egress_intrinsic_metadata_for_output_port_t eg_oport_md) {

    apply {}
}

Pipeline(SwitchIngressParser(),
         SwitchIngress(),
         SwitchIngressDeparser(),
         SwitchEgressParser(),
         SwitchEgress(),
         SwitchEgressDeparser()) pipe;

Switch(pipe) main;
    )";

    auto blk = TestCode(TestCode::Hdr::Tofino2arch, prog, {});
    blk.flags(TrimWhiteSpace | TrimAnnotations);

    // Disable alt phv
    BackendOptions().alt_phv_alloc = false;

    EXPECT_TRUE(blk.CreateBackend());
    EXPECT_TRUE(blk.apply_pass(TestCode::Pass::FullBackend));

    // Expect to have a split_0 state
    auto res = blk.match(TestCode::CodeBlock::ParserIAsm,
                    CheckList{"parser ingress: start",
                              "`.*`", "$entry_point.start.$split_0:"});
    EXPECT_TRUE(res.success) << " pos=" << res.pos << " count=" << res.count << "\n'"
                             << blk.extract_code(TestCode::CodeBlock::ParserIAsm) << "'\n";

    // Expect to *not* have a split_1 state. If the extracts are being correctly reordered, then
    // they should all be able to be procedded in the split_0 state, so additional splitting is not
    // needed.
    //
    // FIXME: disabled for now. Need to improve extractor usage estimate (see split_parser_state),
    // but this needs to be consistent across multiple different passes and the assembler.)
    // res = blk.match(TestCode::CodeBlock::ParserIAsm,
    //                 CheckList{"parser ingress: start",
    //                           "`.*`", "$entry_point.start.$split_1:"});
    // EXPECT_FALSE(res.success) << " pos=" << res.pos << " count=" << res.count << "\n'"
    //                           << blk.extract_code(TestCode::CodeBlock::ParserIAsm) << "'\n";

    // FIXME: Temporary test: MW0/1 should be in the $split_0 state and W0 should be in the split_1
    // state.
    res = blk.match(TestCode::CodeBlock::ParserIAsm,
                    CheckList{"parser ingress: start",
                              "`.*`", "$entry_point.start.$split_0:",
                              "`.*`", "$entry_point.start.$split_1:",
                              "`.*`", ": W0"});
    EXPECT_TRUE(res.success) << " pos=" << res.pos << " count=" << res.count << "\n'"
                             << blk.extract_code(TestCode::CodeBlock::ParserIAsm) << "'\n";

    res = blk.match(TestCode::CodeBlock::ParserIAsm,
                    CheckList{"parser ingress: start",
                              "`.*`", "$entry_point.start.$split_0:",
                              "`.*`", ": MW0",
                              "`.*`", "$entry_point.start.$split_1:"});
    EXPECT_TRUE(res.success) << " pos=" << res.pos << " count=" << res.count << "\n'"
                             << blk.extract_code(TestCode::CodeBlock::ParserIAsm) << "'\n";

    res = blk.match(TestCode::CodeBlock::ParserIAsm,
                    CheckList{"parser ingress: start",
                              "`.*`", "$entry_point.start.$split_0:",
                              "`.*`", ": MW1",
                              "`.*`", "$entry_point.start.$split_1:"});
    EXPECT_TRUE(res.success) << " pos=" << res.pos << " count=" << res.count << "\n'"
                             << blk.extract_code(TestCode::CodeBlock::ParserIAsm) << "'\n";
}

}  // namespace P4::Test
