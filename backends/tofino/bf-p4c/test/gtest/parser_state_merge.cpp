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
 * Tests related to the merging of parser states.
 */

#include <fstream>
#include <regex>
#include <sstream>
#include <string>

#include "bf-p4c/test/gtest/tofino_gtest_utils.h"
#include "bf_gtest_helpers.h"
#include "gtest/gtest.h"
#include "ir/ir.h"
#include "test/gtest/helpers.h"

namespace P4::Test {

/*
 * Verify that compiler-generated stall parser states are not merged during
 * the MergeLoweredParserStates pass.
 *
 * These are the states that contain the following in their name:
 *
 *          $stall_
 *          $ctr_stall_
 *          $oob_stall_
 *          $hdr_len_stop_stall_
 */
TEST(ParserStateMergeTest, DoNotMergeStallStates) {
    // P4 program
    const char *p4_prog = R"(
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

header hdr_variable_t{
    bit<8>  opt_len;
    bit<8>  pad0;
    bit<16> pad1;
};

header hdr40b_t {
    bit<32> f1;
    bit<32> f2;
    bit<32> f3;
    bit<32> f4;
    bit<32> f5;
    bit<32> f6;
    bit<32> f7;
    bit<32> f8;
    bit<32> f9;
    bit<32> reject;
};

struct headers_t {
    ethernet_t eth;
    hdr32b_t h1;
    hdr_variable_t h2;
    hdr40b_t h3;
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
        transition accept;
    }
}

parser EgressParser(
    packet_in pkt,
    out headers_t hdr,
    out metadata_t meta,
    out egress_intrinsic_metadata_t eg_intr_md) {

    ParserCounter<>() pctr;

    state start {
        transition parse_ethernet;
    }

    state parse_ethernet {
        pkt.extract(hdr.eth);
        transition select(hdr.eth.ethertype) {
            0xcdef : parse_h1;
            0x1111 : parse_h2;
            0x2222 : parse_h3;
            default : accept;
        }
    }

    state parse_h1 {
        // create $stall
        pkt.advance(320);
        transition accept;
    }

    state parse_h2 {
        pkt.extract(hdr.h2);
        // create $ctr_stall
        pctr.set(hdr.h2.opt_len);
        transition select(pctr.is_zero()) {
            true : accept;
            false : next_option;
        }
    }

    @dont_unroll
    state next_option {
        pctr.decrement(1);
        transition select(pctr.is_zero()) {
            true : accept;
            false : next_option;
        }
    }

    state parse_h3 {
        // create $oob_stall.
        pkt.extract (hdr.h3);
        transition select(hdr.h3.reject) {
            0x1 : accept;
            0x2 : accept;
            default : reject;
        }
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

    // TestCode disables min depth limits by default. Re-enable.
    BackendOptions().disable_parse_min_depth_limit = false;

    EXPECT_TRUE(blk.CreateBackend());
    EXPECT_TRUE(blk.apply_pass(TestCode::Pass::FullBackend));

    // Check that stall states are present in the output file.
    auto res = blk.match(TestCode::CodeBlock::ParserEAsm,
                         Match::CheckList{"parser egress: start", "`.*`", "parse_h1.$stall_0:"});
    EXPECT_TRUE(res.success) << " pos=" << res.pos << " count=" << res.count << "\n'"
                             << blk.extract_code(TestCode::CodeBlock::ParserEAsm) << "'\n";

    res = blk.match(
        TestCode::CodeBlock::ParserEAsm,
        Match::CheckList{"parser egress: start", "`.*`", "parse_h2.$split_0.$ctr_stall0:"});
    EXPECT_TRUE(res.success) << " pos=" << res.pos << " count=" << res.count << "\n'"
                             << blk.extract_code(TestCode::CodeBlock::ParserEAsm) << "'\n";

    res = blk.match(TestCode::CodeBlock::ParserEAsm,
                    Match::CheckList{"parser egress: start", "`.*`", "parse_h3.$oob_stall_0:"});
    EXPECT_TRUE(res.success) << " pos=" << res.pos << " count=" << res.count << "\n'"
                             << blk.extract_code(TestCode::CodeBlock::ParserEAsm) << "'\n";
}

/*
 *           state parse_seg_list could mistakenly be merged because
 *           the loop created along with state parse_segment was
 *           overlooked in the code.  This test makes sure this
 *           is not the case.
 */
TEST(ParserStateMergeTest, ConsiderLoopInMergeDecision) {
    const char *p4_prog = R"(
header segment_t {
    bit<128> sid;
}

struct metadata {
}

struct headers {
    segment_t[10]  seg_list;
}

parser ParserI(packet_in packet,
                         out headers hdr,
                         out metadata meta,
                         out ingress_intrinsic_metadata_t ig_intr_md) {
    ParserCounter() pctr;

    state start {
        packet.extract(ig_intr_md);
        packet.advance(PORT_METADATA_SIZE);
        pctr.set(8w4);
        transition parse_segments;
    }

    state parse_seg_list {
        transition select(pctr.is_zero()) {
            true: accept;
            false: parse_segments;
        }
    }

    @dont_unroll
    state parse_segments {
        packet.extract(hdr.seg_list.next);
        pctr.decrement(1);
        transition parse_seg_list;
    }
}

parser ParserE(packet_in packet,
                        out headers hdr,
                        out metadata meta,
                        out egress_intrinsic_metadata_t eg_intr_md) {
    state start {
        packet.extract(eg_intr_md);
        transition accept;
    }
}

control IngressP(
        inout headers hdr,
        inout metadata meta,
        in ingress_intrinsic_metadata_t ig_intr_md,
        in ingress_intrinsic_metadata_from_parser_t ig_intr_prsr_md,
        inout ingress_intrinsic_metadata_for_deparser_t ig_intr_dprs_md,
        inout ingress_intrinsic_metadata_for_tm_t ig_intr_tm_md) {
    apply {
            ig_intr_tm_md.ucast_egress_port = 2;
    }
}

control DeparserI(packet_out packet,
                  inout headers hdr,
                  in metadata meta,
                  in ingress_intrinsic_metadata_for_deparser_t ig_intr_md_for_dprsr) {
    apply {
        packet.emit(hdr.seg_list);
    }
}

control EgressP(
        inout headers hdr,
        inout metadata meta,
        in egress_intrinsic_metadata_t eg_intr_md,
        in egress_intrinsic_metadata_from_parser_t eg_intr_prsr_md,
        inout egress_intrinsic_metadata_for_deparser_t ig_intr_dprs_md,
        inout egress_intrinsic_metadata_for_output_port_t eg_intr_oport_md) {
    apply { }
}

control DeparserE(packet_out packet,
                           inout headers hdr,
                           in metadata meta,
                           in egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprsr) {
    apply { }
}

Pipeline(ParserI(),
         IngressP(),
         DeparserI(),
         ParserE(),
         EgressP(),
         DeparserE()) pipe;

Switch(pipe) main;

)";

    auto blk = TestCode(TestCode::Hdr::Tofino1arch, p4_prog);
    blk.flags(Match::TrimWhiteSpace | Match::TrimAnnotations);

    // TestCode disables min depth limits by default. Re-enable.
    BackendOptions().disable_parse_min_depth_limit = false;

    EXPECT_TRUE(blk.CreateBackend());
    EXPECT_TRUE(blk.apply_pass(TestCode::Pass::FullBackend));

    // Check that state parse_seg_list is still present in the output file.
    auto res = blk.match(TestCode::CodeBlock::ParserIAsm,
                         Match::CheckList{"parser ingress: start", "`.*`", "parse_seg_list:"});
    EXPECT_TRUE(res.success) << " pos=" << res.pos << " count=" << res.count << "\n'"
                             << blk.extract_code(TestCode::CodeBlock::ParserEAsm) << "'\n";
}

/**
 * This test checks that correct state is dumped in phv.json for header fields
 * in case the state which they are extracted in is merged.
 */
TEST(ParserStateMergeTest, CheckStateInPhvJsonAfterMerge) {
    const char *p4_prog = R"(
const bit<16> ETHERTYPE_TPID = 0x8100;
const bit<16> ETHERTYPE_IPV4 = 0x0800;
const bit<16> ETHERTYPE_IPV6 = 0x86DD;
header ethernet_h {
    bit<48>  dst_addr;
    bit<48>  src_addr;
    bit<16>  ether_type;
}
header vlan_tag_h {
    bit<3>   pcp;
    bit<1>   cfi;
    bit<12>  vid;
    bit<16>  ether_type;
}
header ipv4_h {
    bit<4>   version;
    bit<4>   ihl;
    bit<8>   diffserv;
    bit<16>  total_len;
    bit<16>  identification;
    bit<3>   flags;
    bit<13>  frag_offset;
    bit<8>   ttl;
    bit<8>   protocol;
    bit<16>  hdr_checksum;
    bit<32>  src_addr;
    bit<32>  dst_addr;
}
header ipv4_options_h {
    varbit<320> data;
}
header ipv6_h {
    bit<4>   version;
    bit<8>   traffic_class;
    bit<20>  flow_label;
    bit<16>  payload_len;
    bit<8>   next_hdr;
    bit<8>   hop_limit;
    bit<128> src_addr;
    bit<128> dst_addr;
}

struct my_ingress_headers_t {
    ethernet_h         ethernet;
    vlan_tag_h[2]      vlan_tag;
    ipv4_h             ipv4;
    ipv4_options_h     ipv4_options;
    ipv6_h             ipv6;
}
struct my_ingress_metadata_t {
    bit<1>        ipv4_csum_err;
}
parser IngressParser(packet_in      pkt,
    out my_ingress_headers_t          hdr,
    out my_ingress_metadata_t         meta,
    out ingress_intrinsic_metadata_t  ig_intr_md)
{
    Checksum() ipv4_checksum;
    state start {
        pkt.extract(ig_intr_md);
        pkt.advance(PORT_METADATA_SIZE);
        transition meta_init;
    }
    state meta_init {
        meta.ipv4_csum_err = 0;
        transition parse_ethernet;
    }
    state parse_ethernet {
        pkt.extract(hdr.ethernet);
        transition select(hdr.ethernet.ether_type) {
            ETHERTYPE_TPID :  parse_vlan_tag;
            ETHERTYPE_IPV4 :  parse_ipv4;
            ETHERTYPE_IPV6 :  parse_ipv6;
            default        :  accept;
        }
    }
    state parse_vlan_tag {
        pkt.extract(hdr.vlan_tag.next);
        transition select(hdr.vlan_tag.last.ether_type) {
            ETHERTYPE_TPID :  parse_vlan_tag;
            ETHERTYPE_IPV4 :  parse_ipv4;
            ETHERTYPE_IPV6 :  parse_ipv6;
            default: accept;
        }
    }
    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        ipv4_checksum.add(hdr.ipv4);
        transition select(hdr.ipv4.ihl) {
            0x5 : parse_ipv4_no_options;
            0x6 &&& 0xE : parse_ipv4_options;
            0x8 &&& 0x8 : parse_ipv4_options;
            default: reject;
        }
    }
    state parse_ipv4_options {
        pkt.extract(
            hdr.ipv4_options,
            ((bit<32>)hdr.ipv4.ihl - 32w5) * 32);
        ipv4_checksum.add(hdr.ipv4_options);
        transition parse_ipv4_no_options;
    }
    state parse_ipv4_no_options {
        meta.ipv4_csum_err = (bit<1>)ipv4_checksum.verify();
        transition accept;
    }
    state parse_ipv6 {
        pkt.extract(hdr.ipv6);
        transition accept;
    }
}
control Ingress(
    inout my_ingress_headers_t                       hdr,
    inout my_ingress_metadata_t                      meta,
    in    ingress_intrinsic_metadata_t               ig_intr_md,
    in    ingress_intrinsic_metadata_from_parser_t   ig_prsr_md,
    inout ingress_intrinsic_metadata_for_deparser_t  ig_dprsr_md,
    inout ingress_intrinsic_metadata_for_tm_t        ig_tm_md)
{
    apply {
    }
}
control IngressDeparser(packet_out pkt,
    inout my_ingress_headers_t                       hdr,
    in    my_ingress_metadata_t                      meta,
    in    ingress_intrinsic_metadata_for_deparser_t  ig_dprsr_md)
{
    Checksum() ipv4_checksum;
    apply {
        hdr.ipv4.hdr_checksum = ipv4_checksum.update({
                hdr.ipv4.version,
                hdr.ipv4.ihl,
                hdr.ipv4.diffserv,
                hdr.ipv4.total_len,
                hdr.ipv4.identification,
                hdr.ipv4.flags,
                hdr.ipv4.frag_offset,
                hdr.ipv4.ttl,
                hdr.ipv4.protocol,
                hdr.ipv4.src_addr,
                hdr.ipv4.dst_addr,
                hdr.ipv4_options.data
            });
        pkt.emit(hdr);
    }
}

struct my_egress_headers_t {
}
struct my_egress_metadata_t {
}
parser EgressParser(packet_in      pkt,
    out my_egress_headers_t          hdr,
    out my_egress_metadata_t         meta,
    out egress_intrinsic_metadata_t  eg_intr_md)
{
    state start {
        pkt.extract(eg_intr_md);
        transition accept;
    }
}
control Egress(
    inout my_egress_headers_t                          hdr,
    inout my_egress_metadata_t                         meta,
    in    egress_intrinsic_metadata_t                  eg_intr_md,
    in    egress_intrinsic_metadata_from_parser_t      eg_prsr_md,
    inout egress_intrinsic_metadata_for_deparser_t     eg_dprsr_md,
    inout egress_intrinsic_metadata_for_output_port_t  eg_oport_md)
{
    apply {
    }
}
control EgressDeparser(packet_out pkt,
    inout my_egress_headers_t                       hdr,
    in    my_egress_metadata_t                      meta,
    in    egress_intrinsic_metadata_for_deparser_t  eg_dprsr_md)
{
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
    EgressDeparser()
) pipe;

Switch(pipe) main;
)";

    std::string output_dir = "ParserStateMerge";
    std::string phv_json = output_dir + "/pipe/logs/phv.json";

    auto blk = TestCode(TestCode::Hdr::Tofino1arch, p4_prog, {}, "",
                        {"-o", output_dir, "-g", "--verbose"});
    blk.flags(Match::TrimWhiteSpace | Match::TrimAnnotations);
    blk.set_phv_log_file(phv_json);

    // TestCode disables min depth limits by default. Re-enable.
    BackendOptions().disable_parse_min_depth_limit = false;

    EXPECT_TRUE(blk.CreateBackend());
    EXPECT_TRUE(blk.apply_pass(TestCode::Pass::FullBackend));
    EXPECT_TRUE(blk.apply_pass(TestCode::Pass::PhvLogging));

    // Check state names in phv.json
    std::ifstream phv_json_stream(phv_json);
    EXPECT_TRUE(phv_json_stream);
    std::stringstream ss;
    ss << phv_json_stream.rdbuf();

    // There are multiple header fields which we could check but we check only 1 of them
    // for simplicity.
    // Only "field_name" and "state" are relevant in this test, irrelevant parts are wildcarded.
    std::regex phv_regex_struct(
        ".*\\{\\n"
        " *\"field_slice\": \\{\\n"
        " *\"field_name\": \"hdr.ipv4_options_h_ipv4_options_data_32b.field\",\\n"
        ".*\\n.*\\n.*\\n.*\\n"  // "slice_info"
        " *\\},\\n"
        ".*\\n"                                     // "phv_number"
        ".*\\n.*\\n.*\\n.*\\n.*\\n.*\\n.*\\n.*\\n"  // "reads"
        ".*\\n.*\\n.*\\n.*\\n"                      // "slice_info"
        " *\"writes\": \\[\\n"
        " *\\{\\n"
        " *\"location\": \\{\\n"
        " *\"detail\": \"ibuf\",\\n"
        " *\"state\": \"parse_ipv4_options\",\\n"
        " *\"type\": \"parser\"\\n"
        " *\\}\\n"
        " *\\}\\n"
        " *\\],\\n"
        ".*\\n.*\\n.*\\n"  // "mutually_exclusive_with"
        " *\\},\\n");

    EXPECT_TRUE(std::regex_search(ss.str(), phv_regex_struct));
}

}  // namespace P4::Test
