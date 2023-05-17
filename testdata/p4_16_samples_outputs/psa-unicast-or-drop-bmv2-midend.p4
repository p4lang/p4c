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
    @noWarn("unused") @name(".send_to_port") action send_to_port_0() {
        ostd.drop = false;
        ostd.multicast_group = 32w0;
        ostd.egress_port = (bit<32>)hdr.ethernet.dstAddr;
    }
    @noWarn("unused") @name(".ingress_drop") action ingress_drop_0() {
        ostd.drop = true;
    }
    @hidden action psaunicastordropbmv2l109() {
        ostd.class_of_service = (bit<8>)hdr.ethernet.srcAddr[0:0];
    }
    @hidden table tbl_send_to_port {
        actions = {
            send_to_port_0();
        }
        const default_action = send_to_port_0();
    }
    @hidden table tbl_ingress_drop {
        actions = {
            ingress_drop_0();
        }
        const default_action = ingress_drop_0();
    }
    @hidden table tbl_psaunicastordropbmv2l109 {
        actions = {
            psaunicastordropbmv2l109();
        }
        const default_action = psaunicastordropbmv2l109();
    }
    apply {
        tbl_send_to_port.apply();
        if (hdr.ethernet.dstAddr == 48w0) {
            tbl_ingress_drop.apply();
        }
        tbl_psaunicastordropbmv2l109.apply();
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
    @hidden action psaunicastordropbmv2l56() {
        hdr.output_data.word2 = 32w1;
    }
    @hidden action psaunicastordropbmv2l58() {
        hdr.output_data.word2 = 32w2;
    }
    @hidden action psaunicastordropbmv2l60() {
        hdr.output_data.word2 = 32w3;
    }
    @hidden action psaunicastordropbmv2l62() {
        hdr.output_data.word2 = 32w4;
    }
    @hidden action psaunicastordropbmv2l64() {
        hdr.output_data.word2 = 32w5;
    }
    @hidden action psaunicastordropbmv2l66() {
        hdr.output_data.word2 = 32w6;
    }
    @hidden action psaunicastordropbmv2l68() {
        hdr.output_data.word2 = 32w7;
    }
    @hidden action psaunicastordropbmv2l54() {
        hdr.output_data.word0 = (bit<32>)istd.egress_port;
        hdr.output_data.word1 = (bit<32>)(bit<16>)istd.instance;
        hdr.output_data.word2 = 32w8;
    }
    @hidden action psaunicastordropbmv2l137() {
        hdr.output_data.word3 = (bit<32>)(bit<8>)istd.class_of_service;
    }
    @hidden table tbl_psaunicastordropbmv2l54 {
        actions = {
            psaunicastordropbmv2l54();
        }
        const default_action = psaunicastordropbmv2l54();
    }
    @hidden table tbl_psaunicastordropbmv2l56 {
        actions = {
            psaunicastordropbmv2l56();
        }
        const default_action = psaunicastordropbmv2l56();
    }
    @hidden table tbl_psaunicastordropbmv2l58 {
        actions = {
            psaunicastordropbmv2l58();
        }
        const default_action = psaunicastordropbmv2l58();
    }
    @hidden table tbl_psaunicastordropbmv2l60 {
        actions = {
            psaunicastordropbmv2l60();
        }
        const default_action = psaunicastordropbmv2l60();
    }
    @hidden table tbl_psaunicastordropbmv2l62 {
        actions = {
            psaunicastordropbmv2l62();
        }
        const default_action = psaunicastordropbmv2l62();
    }
    @hidden table tbl_psaunicastordropbmv2l64 {
        actions = {
            psaunicastordropbmv2l64();
        }
        const default_action = psaunicastordropbmv2l64();
    }
    @hidden table tbl_psaunicastordropbmv2l66 {
        actions = {
            psaunicastordropbmv2l66();
        }
        const default_action = psaunicastordropbmv2l66();
    }
    @hidden table tbl_psaunicastordropbmv2l68 {
        actions = {
            psaunicastordropbmv2l68();
        }
        const default_action = psaunicastordropbmv2l68();
    }
    @hidden table tbl_psaunicastordropbmv2l137 {
        actions = {
            psaunicastordropbmv2l137();
        }
        const default_action = psaunicastordropbmv2l137();
    }
    apply {
        tbl_psaunicastordropbmv2l54.apply();
        if (istd.packet_path == PSA_PacketPath_t.NORMAL) {
            tbl_psaunicastordropbmv2l56.apply();
        } else if (istd.packet_path == PSA_PacketPath_t.NORMAL_UNICAST) {
            tbl_psaunicastordropbmv2l58.apply();
        } else if (istd.packet_path == PSA_PacketPath_t.NORMAL_MULTICAST) {
            tbl_psaunicastordropbmv2l60.apply();
        } else if (istd.packet_path == PSA_PacketPath_t.CLONE_I2E) {
            tbl_psaunicastordropbmv2l62.apply();
        } else if (istd.packet_path == PSA_PacketPath_t.CLONE_E2E) {
            tbl_psaunicastordropbmv2l64.apply();
        } else if (istd.packet_path == PSA_PacketPath_t.RESUBMIT) {
            tbl_psaunicastordropbmv2l66.apply();
        } else if (istd.packet_path == PSA_PacketPath_t.RECIRCULATE) {
            tbl_psaunicastordropbmv2l68.apply();
        }
        tbl_psaunicastordropbmv2l137.apply();
    }
}

control IngressDeparserImpl(packet_out buffer, out empty_metadata_t clone_i2e_meta, out empty_metadata_t resubmit_meta, out empty_metadata_t normal_meta, inout headers_t hdr, in metadata_t meta, in psa_ingress_output_metadata_t istd) {
    @hidden action psaunicastordropbmv2l145() {
        buffer.emit<ethernet_t>(hdr.ethernet);
        buffer.emit<output_data_t>(hdr.output_data);
    }
    @hidden table tbl_psaunicastordropbmv2l145 {
        actions = {
            psaunicastordropbmv2l145();
        }
        const default_action = psaunicastordropbmv2l145();
    }
    apply {
        tbl_psaunicastordropbmv2l145.apply();
    }
}

control EgressDeparserImpl(packet_out buffer, out empty_metadata_t clone_e2e_meta, out empty_metadata_t recirculate_meta, inout headers_t hdr, in metadata_t meta, in psa_egress_output_metadata_t istd, in psa_egress_deparser_input_metadata_t edstd) {
    @hidden action psaunicastordropbmv2l145_0() {
        buffer.emit<ethernet_t>(hdr.ethernet);
        buffer.emit<output_data_t>(hdr.output_data);
    }
    @hidden table tbl_psaunicastordropbmv2l145_0 {
        actions = {
            psaunicastordropbmv2l145_0();
        }
        const default_action = psaunicastordropbmv2l145_0();
    }
    apply {
        tbl_psaunicastordropbmv2l145_0.apply();
    }
}

IngressPipeline<headers_t, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(IngressParserImpl(), cIngress(), IngressDeparserImpl()) ip;
EgressPipeline<headers_t, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(EgressParserImpl(), cEgress(), EgressDeparserImpl()) ep;
PSA_Switch<headers_t, metadata_t, headers_t, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
