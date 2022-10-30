#include <core.p4>
#include <bmv2/psa.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct empty_metadata_t {
}

struct metadata_t {
}

struct headers_t {
    ethernet_t ethernet;
}

parser ingressParserImpl(packet_in packet, out headers_t hdr, inout metadata_t meta, in psa_ingress_parser_input_metadata_t istd, in empty_metadata_t resubmit_meta, in empty_metadata_t recirculate_meta) {
    state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
}

control ingressImpl(inout headers_t hdr, inout metadata_t meta, in psa_ingress_input_metadata_t istd, inout psa_ingress_output_metadata_t ostd) {
    @hidden action psatoplevelassignmentsbmv2l57() {
        ostd.drop = false;
        ostd.egress_port = 32w3;
    }
    @hidden table tbl_psatoplevelassignmentsbmv2l57 {
        actions = {
            psatoplevelassignmentsbmv2l57();
        }
        const default_action = psatoplevelassignmentsbmv2l57();
    }
    apply {
        tbl_psatoplevelassignmentsbmv2l57.apply();
    }
}

parser egressParserImpl(packet_in packet, out headers_t hdr, inout metadata_t meta, in psa_egress_parser_input_metadata_t istd, in empty_metadata_t normal_meta, in empty_metadata_t clone_i2e_meta, in empty_metadata_t clone_e2e_meta) {
    state start {
        transition accept;
    }
}

control egressImpl(inout headers_t hdr, inout metadata_t meta, in psa_egress_input_metadata_t istd, inout psa_egress_output_metadata_t ostd) {
    apply {
    }
}

control ingressDeparserImpl(packet_out packet, out empty_metadata_t clone_i2e_meta, out empty_metadata_t resubmit_meta, out empty_metadata_t normal_meta, inout headers_t hdr, in metadata_t meta, in psa_ingress_output_metadata_t istd) {
    @hidden action psatoplevelassignmentsbmv2l87() {
        packet.emit<ethernet_t>(hdr.ethernet);
    }
    @hidden table tbl_psatoplevelassignmentsbmv2l87 {
        actions = {
            psatoplevelassignmentsbmv2l87();
        }
        const default_action = psatoplevelassignmentsbmv2l87();
    }
    apply {
        tbl_psatoplevelassignmentsbmv2l87.apply();
    }
}

control egressDeparserImpl(packet_out packet, out empty_metadata_t clone_e2e_meta, out empty_metadata_t recirculate_meta, inout headers_t hdr, in metadata_t meta, in psa_egress_output_metadata_t istd, in psa_egress_deparser_input_metadata_t edstd) {
    @hidden action psatoplevelassignmentsbmv2l87_0() {
        packet.emit<ethernet_t>(hdr.ethernet);
    }
    @hidden table tbl_psatoplevelassignmentsbmv2l87_0 {
        actions = {
            psatoplevelassignmentsbmv2l87_0();
        }
        const default_action = psatoplevelassignmentsbmv2l87_0();
    }
    apply {
        tbl_psatoplevelassignmentsbmv2l87_0.apply();
    }
}

IngressPipeline<headers_t, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(ingressParserImpl(), ingressImpl(), ingressDeparserImpl()) ip;
EgressPipeline<headers_t, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(egressParserImpl(), egressImpl(), egressDeparserImpl()) ep;
PSA_Switch<headers_t, metadata_t, headers_t, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
