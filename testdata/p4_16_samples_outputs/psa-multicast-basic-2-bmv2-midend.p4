#include <core.p4>
#include <bmv2/psa.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header output_data_t {
    bit<32> word0;
    bit<32> word1;
    bit<32> word2;
    bit<32> word3;
}

struct empty_metadata_t {
}

struct metadata_t {
}

struct headers_t {
    ethernet_t    ethernet;
    output_data_t output_data;
}

parser IngressParserImpl(packet_in pkt, out headers_t hdr, inout metadata_t user_meta, in psa_ingress_parser_input_metadata_t istd, in empty_metadata_t resubmit_meta, in empty_metadata_t recirculate_meta) {
    state start {
        pkt.extract<ethernet_t>(hdr.ethernet);
        pkt.extract<output_data_t>(hdr.output_data);
        transition accept;
    }
}

control cIngress(inout headers_t hdr, inout metadata_t user_meta, in psa_ingress_input_metadata_t istd, inout psa_ingress_output_metadata_t ostd) {
    @noWarn("unused") @name(".multicast") action multicast_0() {
        ostd.drop = false;
        ostd.multicast_group = (bit<32>)hdr.ethernet.dstAddr;
    }
    @hidden action psamulticastbasic2bmv2l98() {
        ostd.class_of_service = (bit<8>)hdr.ethernet.srcAddr[0:0];
    }
    @hidden table tbl_multicast {
        actions = {
            multicast_0();
        }
        const default_action = multicast_0();
    }
    @hidden table tbl_psamulticastbasic2bmv2l98 {
        actions = {
            psamulticastbasic2bmv2l98();
        }
        const default_action = psamulticastbasic2bmv2l98();
    }
    apply {
        tbl_multicast.apply();
        tbl_psamulticastbasic2bmv2l98.apply();
    }
}

parser EgressParserImpl(packet_in pkt, out headers_t hdr, inout metadata_t user_meta, in psa_egress_parser_input_metadata_t istd, in empty_metadata_t normal_meta, in empty_metadata_t clone_i2e_meta, in empty_metadata_t clone_e2e_meta) {
    state start {
        pkt.extract<ethernet_t>(hdr.ethernet);
        pkt.extract<output_data_t>(hdr.output_data);
        transition accept;
    }
}

control cEgress(inout headers_t hdr, inout metadata_t user_meta, in psa_egress_input_metadata_t istd, inout psa_egress_output_metadata_t ostd) {
    @hidden action psamulticastbasic2bmv2l56() {
        hdr.output_data.word2 = 32w1;
    }
    @hidden action psamulticastbasic2bmv2l58() {
        hdr.output_data.word2 = 32w2;
    }
    @hidden action psamulticastbasic2bmv2l60() {
        hdr.output_data.word2 = 32w3;
    }
    @hidden action psamulticastbasic2bmv2l62() {
        hdr.output_data.word2 = 32w4;
    }
    @hidden action psamulticastbasic2bmv2l64() {
        hdr.output_data.word2 = 32w5;
    }
    @hidden action psamulticastbasic2bmv2l66() {
        hdr.output_data.word2 = 32w6;
    }
    @hidden action psamulticastbasic2bmv2l68() {
        hdr.output_data.word2 = 32w7;
    }
    @hidden action psamulticastbasic2bmv2l54() {
        hdr.output_data.word0 = (bit<32>)istd.egress_port;
        hdr.output_data.word1 = (bit<32>)(bit<16>)istd.instance;
        hdr.output_data.word2 = 32w8;
    }
    @hidden action psamulticastbasic2bmv2l126() {
        hdr.output_data.word3 = (bit<32>)(bit<8>)istd.class_of_service;
    }
    @hidden table tbl_psamulticastbasic2bmv2l54 {
        actions = {
            psamulticastbasic2bmv2l54();
        }
        const default_action = psamulticastbasic2bmv2l54();
    }
    @hidden table tbl_psamulticastbasic2bmv2l56 {
        actions = {
            psamulticastbasic2bmv2l56();
        }
        const default_action = psamulticastbasic2bmv2l56();
    }
    @hidden table tbl_psamulticastbasic2bmv2l58 {
        actions = {
            psamulticastbasic2bmv2l58();
        }
        const default_action = psamulticastbasic2bmv2l58();
    }
    @hidden table tbl_psamulticastbasic2bmv2l60 {
        actions = {
            psamulticastbasic2bmv2l60();
        }
        const default_action = psamulticastbasic2bmv2l60();
    }
    @hidden table tbl_psamulticastbasic2bmv2l62 {
        actions = {
            psamulticastbasic2bmv2l62();
        }
        const default_action = psamulticastbasic2bmv2l62();
    }
    @hidden table tbl_psamulticastbasic2bmv2l64 {
        actions = {
            psamulticastbasic2bmv2l64();
        }
        const default_action = psamulticastbasic2bmv2l64();
    }
    @hidden table tbl_psamulticastbasic2bmv2l66 {
        actions = {
            psamulticastbasic2bmv2l66();
        }
        const default_action = psamulticastbasic2bmv2l66();
    }
    @hidden table tbl_psamulticastbasic2bmv2l68 {
        actions = {
            psamulticastbasic2bmv2l68();
        }
        const default_action = psamulticastbasic2bmv2l68();
    }
    @hidden table tbl_psamulticastbasic2bmv2l126 {
        actions = {
            psamulticastbasic2bmv2l126();
        }
        const default_action = psamulticastbasic2bmv2l126();
    }
    apply {
        tbl_psamulticastbasic2bmv2l54.apply();
        if (istd.packet_path == PSA_PacketPath_t.NORMAL) {
            tbl_psamulticastbasic2bmv2l56.apply();
        } else if (istd.packet_path == PSA_PacketPath_t.NORMAL_UNICAST) {
            tbl_psamulticastbasic2bmv2l58.apply();
        } else if (istd.packet_path == PSA_PacketPath_t.NORMAL_MULTICAST) {
            tbl_psamulticastbasic2bmv2l60.apply();
        } else if (istd.packet_path == PSA_PacketPath_t.CLONE_I2E) {
            tbl_psamulticastbasic2bmv2l62.apply();
        } else if (istd.packet_path == PSA_PacketPath_t.CLONE_E2E) {
            tbl_psamulticastbasic2bmv2l64.apply();
        } else if (istd.packet_path == PSA_PacketPath_t.RESUBMIT) {
            tbl_psamulticastbasic2bmv2l66.apply();
        } else if (istd.packet_path == PSA_PacketPath_t.RECIRCULATE) {
            tbl_psamulticastbasic2bmv2l68.apply();
        }
        tbl_psamulticastbasic2bmv2l126.apply();
    }
}

control IngressDeparserImpl(packet_out buffer, out empty_metadata_t clone_i2e_meta, out empty_metadata_t resubmit_meta, out empty_metadata_t normal_meta, inout headers_t hdr, in metadata_t meta, in psa_ingress_output_metadata_t istd) {
    @hidden action psamulticastbasic2bmv2l134() {
        buffer.emit<ethernet_t>(hdr.ethernet);
        buffer.emit<output_data_t>(hdr.output_data);
    }
    @hidden table tbl_psamulticastbasic2bmv2l134 {
        actions = {
            psamulticastbasic2bmv2l134();
        }
        const default_action = psamulticastbasic2bmv2l134();
    }
    apply {
        tbl_psamulticastbasic2bmv2l134.apply();
    }
}

control EgressDeparserImpl(packet_out buffer, out empty_metadata_t clone_e2e_meta, out empty_metadata_t recirculate_meta, inout headers_t hdr, in metadata_t meta, in psa_egress_output_metadata_t istd, in psa_egress_deparser_input_metadata_t edstd) {
    @hidden action psamulticastbasic2bmv2l134_0() {
        buffer.emit<ethernet_t>(hdr.ethernet);
        buffer.emit<output_data_t>(hdr.output_data);
    }
    @hidden table tbl_psamulticastbasic2bmv2l134_0 {
        actions = {
            psamulticastbasic2bmv2l134_0();
        }
        const default_action = psamulticastbasic2bmv2l134_0();
    }
    apply {
        tbl_psamulticastbasic2bmv2l134_0.apply();
    }
}

IngressPipeline<headers_t, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(IngressParserImpl(), cIngress(), IngressDeparserImpl()) ip;
EgressPipeline<headers_t, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(EgressParserImpl(), cEgress(), EgressDeparserImpl()) ep;
PSA_Switch<headers_t, metadata_t, headers_t, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
