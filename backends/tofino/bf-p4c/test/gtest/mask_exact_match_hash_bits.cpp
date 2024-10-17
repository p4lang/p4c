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

#include <fstream>
#include <regex>
#include <sstream>
#include <string>

#include "gtest/gtest.h"
#include "bf_gtest_helpers.h"

#include "ir/ir.h"
#include "bf-p4c/test/gtest/tofino_gtest_utils.h"
#include "test/gtest/helpers.h"

namespace P4::Test {

/* Tests the complete exclusion of a match field from the exact match
 * hash calculation.  Validates that the excluded field is not in ghost
 * bits but instead specified in match bits and that SRAM allocation
 * has been adjusted accordignly.
 *
 */
TEST(MaskExactMatchHashBitsTest, TotalFieldExclusion) {
// P4 program
const char * p4_prog = R"(
header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> ethertype;
}

header hdr32b_t {
    bit<10> f1;
    bit<2> f2;
    bit<4> f3;
    bit<32> f4;
};

struct headers_t {
    ethernet_t eth;
    hdr32b_t h1;
}

struct metadata_t {
}

parser IngressParser(
    packet_in pkt,
    out headers_t hdr,
    out metadata_t meta,
    out ingress_intrinsic_metadata_t ig_intr_md) {

    state start {
        pkt.extract(ig_intr_md);
        pkt.advance(PORT_METADATA_SIZE);
        transition parse_ethernet;
    }

    state parse_ethernet {
        pkt.extract(hdr.eth);
        transition select(hdr.eth.ethertype) {
            0xcdef : parse_h1;
            default : accept;
        }
    }

    state parse_h1 {
        pkt.extract(hdr.h1);
        transition accept;
    }
}

parser EgressParser(
    packet_in pkt,
    out headers_t hdr,
    out metadata_t meta,
    out egress_intrinsic_metadata_t eg_intr_md) {

    state start {
        pkt.extract(eg_intr_md);
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

    action hit() {
        hdr.eth.dst_addr = hdr.eth.src_addr;
    }

    action hit2() {
        hdr.eth.dst_addr = hdr.eth.src_addr;
    }

    action hit3() {
        hdr.eth.dst_addr = hdr.eth.src_addr;
    }

    action hit4() {
        hdr.eth.dst_addr = hdr.eth.src_addr;
    }

    action hit5() {
        ig_intr_tm_md.ucast_egress_port = 2;
    }

    action hit6(bit<48> eth_dst) {
        hdr.eth.dst_addr = eth_dst;
    }

    action hit7(bit<16> eth_type) {
        hdr.eth.ethertype = eth_type;
    }

    table table1 {
        key = {
            hdr.h1.f1 : exact;
            hdr.h1.f2 : exact @hash_mask(0); // Field excluded completely from hash calculation.
        }

        actions = {
            hit;
            hit2;
            hit3;
            hit4;
            hit5;
            hit7;
            NoAction;
        }

        size = 1024;
    }

    apply {
        table1.apply();
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

    // TestCode disables min depth limits by default. Re-enable.
    BackendOptions().disable_parse_min_depth_limit = false;

    EXPECT_TRUE(blk.CreateBackend());
    EXPECT_TRUE(blk.apply_pass(TestCode::Pass::FullBackend));

    auto res =
        blk.match(TestCode::CodeBlock::MauAsm,
            Match::CheckList{"stage 0 ingress:", "`.*`",
                             "exact_match table1_0 0:", "`.*`",
                             "{ group: 0, index: 0..9, select: 40..51 & 0x0, rams: [[7, 2]] }"});
    EXPECT_TRUE(res.success) << " pos=" << res.pos << " count=" << res.count << "\n'"
                             << blk.extract_code(TestCode::CodeBlock::MauAsm) << "'\n";

    res =
        blk.match(TestCode::CodeBlock::MauAsm,
            Match::CheckList{"stage 0 ingress:", "`.*`",
                             "exact_match table1_0 0:", "`.*`",
                             "input_xbar:",
                               "exact group 0: { 4: hdr.h1.f2, 6: hdr.h1.f1 }",
                               "hash 0:",
                                 "0..1: hdr.h1.f1(0..1)",
                                 "2..9: hdr.h1.f1(2..9)",
                               "hash group 0:",
                                 "table: [0]",
                                 "seed: 0x0"});
    EXPECT_TRUE(res.success) << " pos=" << res.pos << " count=" << res.count << "\n'"
                             << blk.extract_code(TestCode::CodeBlock::MauAsm) << "'\n";

    res =
        blk.match(TestCode::CodeBlock::MauAsm,
            Match::CheckList{"stage 0 ingress:", "`.*`",
                             "exact_match table1_0 0:", "`.*`",
                             "match: [ hdr.h1.f2 ]"});
    EXPECT_TRUE(res.success) << " pos=" << res.pos << " count=" << res.count << "\n'"
                             << blk.extract_code(TestCode::CodeBlock::MauAsm) << "'\n";
}

/* Tests the partial match field exclusion from the hash calculation.
 * Validates that the excluded field bits are not in ghost bits, and that
 * they are specified in match bits.  Also checks that SRAM allocation has
 * been adjusted accordignly.
 *
 */
TEST(MaskExactMatchHashBitsTest, PartialFieldExclusion) {
// P4 program
const char * p4_prog = R"(
header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> ethertype;
}

header hdr32b_t {
    bit<10> f1;
    bit<2> f2;
    bit<4> f3;
    bit<32> f4;
};

struct headers_t {
    ethernet_t eth;
    hdr32b_t h1;
}

struct metadata_t {
}

parser IngressParser(
    packet_in pkt,
    out headers_t hdr,
    out metadata_t meta,
    out ingress_intrinsic_metadata_t ig_intr_md) {

    state start {
        pkt.extract(ig_intr_md);
        pkt.advance(PORT_METADATA_SIZE);
        transition parse_ethernet;
    }

    state parse_ethernet {
        pkt.extract(hdr.eth);
        transition select(hdr.eth.ethertype) {
            0xcdef : parse_h1;
            default : accept;
        }
    }

    state parse_h1 {
        pkt.extract(hdr.h1);
        transition accept;
    }
}

parser EgressParser(
    packet_in pkt,
    out headers_t hdr,
    out metadata_t meta,
    out egress_intrinsic_metadata_t eg_intr_md) {

    state start {
        pkt.extract(eg_intr_md);
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

    action hit() {
        hdr.eth.dst_addr = hdr.eth.src_addr;
    }

    action hit2() {
        hdr.eth.dst_addr = hdr.eth.src_addr;
    }

    action hit3() {
        hdr.eth.dst_addr = hdr.eth.src_addr;
    }

    action hit4() {
        hdr.eth.dst_addr = hdr.eth.src_addr;
    }

    action hit5() {
        hdr.eth.dst_addr = hdr.eth.src_addr;
    }

    action hit6(bit<48> eth_dst) {
        hdr.eth.dst_addr = eth_dst;
    }

    action hit7(bit<16> eth_type) {
        hdr.eth.ethertype = eth_type;
    }

    table table1 {
        key = {
            hdr.h1.f1 : exact @hash_mask(0b1100011110);  // Bits set to 0 are excluded from hash
                                                         // calculation (bits 0, 5, 6 and 7).
            hdr.h1.f2 : exact;
        }

        actions = {
            hit;
            hit2;
            hit3;
            hit4;
            hit5;
            //hit6;
            hit7;
            NoAction;
        }

        size = 1024;
    }

    table table2 {
        key = {
            hdr.h1.f3 : exact;
            hdr.h1.f4 : exact;
        }

        actions = {
            hit;
            NoAction;
        }

        size = 4096;
    }

    apply {
        table1.apply();
        table2.apply();
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

    // TestCode disables min depth limits by default. Re-enable.
    BackendOptions().disable_parse_min_depth_limit = false;

    EXPECT_TRUE(blk.CreateBackend());
    EXPECT_TRUE(blk.apply_pass(TestCode::Pass::FullBackend));

    auto res =
        blk.match(TestCode::CodeBlock::MauAsm,
            Match::CheckList{"stage 0 ingress:", "`.*`",
                             "exact_match table1_0 0:", "`.*`",
                             "{ group: 0, index: 0..9, select: 40..51 & 0x0, rams: [[7, 2]] }"});
    EXPECT_TRUE(res.success) << " pos=" << res.pos << " count=" << res.count << "\n'"
                             << blk.extract_code(TestCode::CodeBlock::MauAsm) << "'\n";

    res =
        blk.match(TestCode::CodeBlock::MauAsm,
            Match::CheckList{"stage 0 ingress:", "`.*`",
                             "exact_match table1_0 0:", "`.*`",
                             "input_xbar:",
                               "exact group 0: { 4: hdr.h1.f2, 6: hdr.h1.f1 }",
                               "hash 0:",
                                 "0..1: hdr.h1.f2",
                                 "2: hdr.h1.f1(1)",
                                 "3..5: hdr.h1.f1(2..4)",
                                 "6..7: hdr.h1.f1(8..9)",
                               "hash group 0:",
                                 "table: [0]",
                                 "seed: 0x0"});
    EXPECT_TRUE(res.success) << " pos=" << res.pos << " count=" << res.count << "\n'"
                             << blk.extract_code(TestCode::CodeBlock::MauAsm) << "'\n";

    res =
        blk.match(TestCode::CodeBlock::MauAsm,
            Match::CheckList{"stage 0 ingress:", "`.*`",
                             "exact_match table1_0 0:", "`.*`",
                             "match: [ hdr.h1.f1(0), hdr.h1.f1(5..7) ]"});
    EXPECT_TRUE(res.success) << " pos=" << res.pos << " count=" << res.count << "\n'"
                             << blk.extract_code(TestCode::CodeBlock::MauAsm) << "'\n";
}

/* Compiles the same P4 code as previously but with no exclusion.
 * Provides the default allocation when @hash_mask is not specified.
 */
TEST(MaskExactMatchHashBitsTest, NoExclusion) {
// P4 program
const char * p4_prog = R"(
header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> ethertype;
}

header hdr32b_t {
    bit<10> f1;
    bit<2> f2;
    bit<4> f3;
    bit<32> f4;
};

struct headers_t {
    ethernet_t eth;
    hdr32b_t h1;
}

struct metadata_t {
}

parser IngressParser(
    packet_in pkt,
    out headers_t hdr,
    out metadata_t meta,
    out ingress_intrinsic_metadata_t ig_intr_md) {

    state start {
        pkt.extract(ig_intr_md);
        pkt.advance(PORT_METADATA_SIZE);
        transition parse_ethernet;
    }

    state parse_ethernet {
        pkt.extract(hdr.eth);
        transition select(hdr.eth.ethertype) {
            0xcdef : parse_h1;
            default : accept;
        }
    }

    state parse_h1 {
        pkt.extract(hdr.h1);
        transition accept;
    }
}

parser EgressParser(
    packet_in pkt,
    out headers_t hdr,
    out metadata_t meta,
    out egress_intrinsic_metadata_t eg_intr_md) {

    state start {
        pkt.extract(eg_intr_md);
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

    action hit() {
        hdr.eth.dst_addr = hdr.eth.src_addr;
    }

    action hit2() {
        hdr.eth.dst_addr = hdr.eth.src_addr;
    }

    action hit3() {
        hdr.eth.dst_addr = hdr.eth.src_addr;
    }

    action hit4() {
        hdr.eth.dst_addr = hdr.eth.src_addr;
    }

    action hit5() {
        ig_intr_tm_md.ucast_egress_port = 2;
    }

    action hit6(bit<48> eth_dst) {
        hdr.eth.dst_addr = eth_dst;
    }

    action hit7(bit<16> eth_type) {
        hdr.eth.ethertype = eth_type;
    }

    table table1 {
        key = {
            hdr.h1.f1 : exact;
            hdr.h1.f2 : exact;
        }

        actions = {
            hit;
            hit2;
            hit3;
            hit4;
            hit5;
            hit7;
            NoAction;
        }

        size = 1024;
    }

    apply {
        table1.apply();
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

    // TestCode disables min depth limits by default. Re-enable.
    BackendOptions().disable_parse_min_depth_limit = false;

    EXPECT_TRUE(blk.CreateBackend());
    EXPECT_TRUE(blk.apply_pass(TestCode::Pass::FullBackend));

    auto res =
        blk.match(TestCode::CodeBlock::MauAsm,
            Match::CheckList{"stage 0 ingress:", "`.*`",
                             "exact_match table1_0 0:", "`.*`",
                    "{ group: 0, index: 0..9, select: 40..51 & 0x1, rams: [[7, 2], [7, 3]] }"});
    EXPECT_TRUE(res.success) << " pos=" << res.pos << " count=" << res.count << "\n'"
                             << blk.extract_code(TestCode::CodeBlock::MauAsm) << "'\n";

    res =
        blk.match(TestCode::CodeBlock::MauAsm,
            Match::CheckList{"stage 0 ingress:", "`.*`",
                             "exact_match table1_0 0:", "`.*`",
                             "input_xbar:",
                               "exact group 0: { 4: hdr.h1.f2, 6: hdr.h1.f1 }",
                               "hash 0:",
                                 "0..1: hdr.h1.f2",
                                 "2..3: hdr.h1.f1(0..1)",
                                 "4..9: hdr.h1.f1(2..7)",
                                 "40: hdr.h1.f1(8)",
                               "hash group 0:",
                                 "table: [0]",
                                 "seed: 0x0"});
    EXPECT_TRUE(res.success) << " pos=" << res.pos << " count=" << res.count << "\n'"
                             << blk.extract_code(TestCode::CodeBlock::MauAsm) << "'\n";

    res =
        blk.match(TestCode::CodeBlock::MauAsm,
            Match::CheckList{"stage 0 ingress:", "`.*`",
                             "exact_match table1_0 0:", "`.*`",
                             "match: [ hdr.h1.f1(9) ]"});
    EXPECT_TRUE(res.success) << " pos=" << res.pos << " count=" << res.count << "\n'"
                             << blk.extract_code(TestCode::CodeBlock::MauAsm) << "'\n";
}

}
