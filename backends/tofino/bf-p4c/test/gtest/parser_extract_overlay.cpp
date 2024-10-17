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

namespace ParserExtractOverlay {

std::string prog() {
    return R"(
// ----------------------------------------------------------------------------
// Common protocols/types
//-----------------------------------------------------------------------------
//#define ETHERTYPE_H1   0x08ff


// ----------------------------------------------------------------------------
// Headers
//-----------------------------------------------------------------------------
typedef bit<48> mac_addr_t;

header ethernet_h {
    mac_addr_t dst_addr;
    mac_addr_t src_addr;
    bit<16> ether_type;
}

header hdr_w_csum_h {
    bit<16> f1;
    bit<16> csum;
}

//-----------------------------------------------------------------------------
// Other Metadata Definitions
//-----------------------------------------------------------------------------
// Flags
struct switch_ingress_flags_t {
    bool h1_checksum_err;
}

// Ingress metadata
struct switch_ingress_metadata_t {
    switch_ingress_flags_t flags;
}

// Egress metadata
struct switch_egress_metadata_t {
}

struct switch_header_t {
    ethernet_h ethernet;
    hdr_w_csum_h  h1;
}

struct switch_port_metadata_t {
}

//=============================================================================
// Ingress parser
//=============================================================================
parser SwitchIngressParser(
        packet_in pkt,
        out switch_header_t hdr,
        out switch_ingress_metadata_t ig_md,
        out ingress_intrinsic_metadata_t ig_intr_md) {
    Checksum() hx_checksum;

    state start {
        pkt.extract(ig_intr_md);
        transition parse_port_metadata;
    }

    state parse_port_metadata {
        // Parse port metadata produced by ibuf
        port_metadata_unpack<switch_port_metadata_t>(pkt);
        transition parse_packet;
    }

    state parse_packet {
        transition parse_ethernet;
    }

    state parse_ethernet {
        pkt.extract(hdr.ethernet);
        transition select(hdr.ethernet.ether_type, ig_intr_md.ingress_port) {
            (0x08ff, _) : parse_h1;
            default : accept;
        }
    }

    state parse_h1 {
        pkt.extract(hdr.h1);

        hx_checksum.add(hdr.h1);
        ig_md.flags.h1_checksum_err = hx_checksum.verify();

        transition accept;
    }

}

//----------------------------------------------------------------------------
// Egress parser
//----------------------------------------------------------------------------
parser SwitchEgressParser(
        packet_in pkt,
        out switch_header_t hdr,
        out switch_egress_metadata_t eg_md,
        out egress_intrinsic_metadata_t eg_intr_md) {

    state start {
        pkt.extract(eg_intr_md);

        transition accept;
    }

}


//-----------------------------------------------------------------------------
// Ingress Deparser
//-----------------------------------------------------------------------------
control SwitchIngressDeparser(
    packet_out pkt,
    inout switch_header_t hdr,
    in switch_ingress_metadata_t ig_md,
    in ingress_intrinsic_metadata_for_deparser_t ig_intr_md_for_dprsr) {

    apply {
        pkt.emit(hdr);
    }
}


//-----------------------------------------------------------------------------
// Egress Deparser
//-----------------------------------------------------------------------------
control SwitchEgressDeparser(
        packet_out pkt,
        inout switch_header_t hdr,
        in switch_egress_metadata_t eg_md,
        in egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprsr) {

    apply { }
}


//-----------------------------------------------------------------------------
// Ingress Switch
//-----------------------------------------------------------------------------
control SwitchIngress(
        inout switch_header_t hdr,
        inout switch_ingress_metadata_t ig_md,
        in ingress_intrinsic_metadata_t ig_intr_md,
        in ingress_intrinsic_metadata_from_parser_t ig_intr_from_prsr,
        inout ingress_intrinsic_metadata_for_deparser_t ig_intr_md_for_dprsr,
        inout ingress_intrinsic_metadata_for_tm_t ig_intr_md_for_tm) {

    apply {
        ig_intr_md_for_tm.ucast_egress_port = ig_intr_md.ingress_port;
        if (!ig_md.flags.h1_checksum_err) {
            if (hdr.h1.isValid()) {
                hdr.h1.f1 = hdr.h1.f1 - 1;
            }
        }
    }
}

//-----------------------------------------------------------------------------
// Egress Switch
//-----------------------------------------------------------------------------
control SwitchEgress(
        inout switch_header_t hdr,
        inout switch_egress_metadata_t eg_md,
        in egress_intrinsic_metadata_t eg_intr_md,
        in egress_intrinsic_metadata_from_parser_t eg_intr_from_prsr,
        inout egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprsr,
        inout egress_intrinsic_metadata_for_output_port_t eg_intr_md_for_oport) {
    apply { }
}

Pipeline(SwitchIngressParser(),
        SwitchIngress(),
        SwitchIngressDeparser(),
        SwitchEgressParser(),
        SwitchEgress(),
        SwitchEgressDeparser()) pipe;

Switch(pipe) main;

    )";
}

}  // namespace ParserExtractOverlay

/** Verify that fields are not packed in with the ingress port.
  * Until a recent fix, checksum validation results were being packed with the
  * ingress port, and these potentially overlap with the version and/or recirc
  * bits.
  */
TEST(ParserExtractOverlay, DontPackWithIngressPort_Traditional) {
    auto blk = TestCode(TestCode::Hdr::Tofino1arch, ParserExtractOverlay::prog(), {});
    blk.flags(TrimWhiteSpace | TrimAnnotations);

    // Disable alt phv
    BackendOptions().alt_phv_alloc = false;

    EXPECT_TRUE(blk.CreateBackend());
    EXPECT_TRUE(blk.apply_pass(TestCode::Pass::FullBackend));

    auto phv_asm = blk.extract_code(TestCode::CodeBlock::PhvAsm);
    auto ingress_port_container = blk.get_field_container("ig_intr_md.ingress_port", phv_asm);
    auto csum_err_container = blk.get_field_container("ig_md.flags.h1_checksum_err", phv_asm);

    EXPECT_NE(ingress_port_container, "");
    EXPECT_NE(csum_err_container, "");

    EXPECT_NE(ingress_port_container, csum_err_container) << " in " << phv_asm;
}

/** Verify that fields are not packed in with the ingress port.
  * Until a recent fix, checksum validation results were being packed with the
  * ingress port, and these potentially overlap with the version and/or recirc
  * bits.
  */
TEST(ParserExtractOverlay, DontPackWithIngressPort_AltPhv) {
    auto blk = TestCode(TestCode::Hdr::Tofino1arch, ParserExtractOverlay::prog(), {});
    blk.flags(TrimWhiteSpace | TrimAnnotations);

    // Disable alt phv
    BackendOptions().alt_phv_alloc = true;

    EXPECT_TRUE(blk.CreateBackend());
    EXPECT_TRUE(blk.apply_pass(TestCode::Pass::FullBackend));

    auto phv_asm = blk.extract_code(TestCode::CodeBlock::PhvAsm);
    auto ingress_port_container = blk.get_field_container("ig_intr_md.ingress_port", phv_asm);
    auto csum_err_container = blk.get_field_container("ig_md.flags.h1_checksum_err", phv_asm);

    EXPECT_NE(ingress_port_container, "");
    EXPECT_NE(csum_err_container, "");

    EXPECT_NE(ingress_port_container, csum_err_container) << " in " << phv_asm;
}

}  // namespace P4::Test
