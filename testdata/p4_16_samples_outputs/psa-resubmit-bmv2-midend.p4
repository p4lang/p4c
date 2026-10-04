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
    ethernet_t ethernet;
    output_data_t output_data;
}

parser IngressParserImpl(packet_in pkt, out headers_t hdr, inout metadata_t user_meta, in psa_ingress_parser_input_metadata_t istd, in empty_metadata_t resubmit_meta, in empty_metadata_t recirculate_meta) {
    state start {
        pkt.extract < ethernet_t >(hdr.ethernet);
        pkt.extract < output_data_t >(hdr.output_data);
        transition accept;
    }
}

control cIngress(inout headers_t hdr, inout metadata_t user_meta, in psa_ingress_input_metadata_t istd, inout psa_ingress_output_metadata_t ostd) {
    @noWarn("unused") @name(".send_to_port") action send_to_port_0() {
        ostd.multicast_group = 32w0;
        ostd.egress_port =(bit<32>) hdr.ethernet.dstAddr;
    }
    @hidden action psaresubmitbmv2l100() {
        hdr.ethernet.srcAddr = 48w256;
        ostd.resubmit = true;
    }
    @hidden action psaresubmitbmv2l106() {
        hdr.ethernet.etherType = 16w0xf00d;
    }
    @hidden action psaresubmitbmv2l53() {
        hdr.output_data.word0 = 32w1;
    }
    @hidden action psaresubmitbmv2l55() {
        hdr.output_data.word0 = 32w2;
    }
    @hidden action psaresubmitbmv2l57() {
        hdr.output_data.word0 = 32w3;
    }
    @hidden action psaresubmitbmv2l59() {
        hdr.output_data.word0 = 32w4;
    }
    @hidden action psaresubmitbmv2l61() {
        hdr.output_data.word0 = 32w5;
    }
    @hidden action psaresubmitbmv2l63() {
        hdr.output_data.word0 = 32w6;
    }
    @hidden action psaresubmitbmv2l65() {
        hdr.output_data.word0 = 32w7;
    }
    @hidden action psaresubmitbmv2l51() {
        hdr.output_data.word0 = 32w8;
    }
    @hidden action psaresubmitbmv2l93() {
        ostd.drop = false;
    }
    @hidden table tbl_psaresubmitbmv2l93 {
        actions = {
            psaresubmitbmv2l93();
        }
        const default_action = psaresubmitbmv2l93();
    }
    @hidden table tbl_psaresubmitbmv2l100 {
        actions = {
            psaresubmitbmv2l100();
        }
        const default_action = psaresubmitbmv2l100();
    }
    @hidden table tbl_psaresubmitbmv2l106 {
        actions = {
            psaresubmitbmv2l106();
        }
        const default_action = psaresubmitbmv2l106();
    }
    @hidden table tbl_send_to_port {
        actions = {
            send_to_port_0();
        }
        const default_action = send_to_port_0();
    }
    @hidden table tbl_psaresubmitbmv2l51 {
        actions = {
            psaresubmitbmv2l51();
        }
        const default_action = psaresubmitbmv2l51();
    }
    @hidden table tbl_psaresubmitbmv2l53 {
        actions = {
            psaresubmitbmv2l53();
        }
        const default_action = psaresubmitbmv2l53();
    }
    @hidden table tbl_psaresubmitbmv2l55 {
        actions = {
            psaresubmitbmv2l55();
        }
        const default_action = psaresubmitbmv2l55();
    }
    @hidden table tbl_psaresubmitbmv2l57 {
        actions = {
            psaresubmitbmv2l57();
        }
        const default_action = psaresubmitbmv2l57();
    }
    @hidden table tbl_psaresubmitbmv2l59 {
        actions = {
            psaresubmitbmv2l59();
        }
        const default_action = psaresubmitbmv2l59();
    }
    @hidden table tbl_psaresubmitbmv2l61 {
        actions = {
            psaresubmitbmv2l61();
        }
        const default_action = psaresubmitbmv2l61();
    }
    @hidden table tbl_psaresubmitbmv2l63 {
        actions = {
            psaresubmitbmv2l63();
        }
        const default_action = psaresubmitbmv2l63();
    }
    @hidden table tbl_psaresubmitbmv2l65 {
        actions = {
            psaresubmitbmv2l65();
        }
        const default_action = psaresubmitbmv2l65();
    }
    apply {
        tbl_psaresubmitbmv2l93.apply();
        if (istd.packet_path != PSA_PacketPath_t.RESUBMIT) {
            tbl_psaresubmitbmv2l100.apply();
        } else {
            tbl_psaresubmitbmv2l106.apply();
            tbl_send_to_port.apply();
            tbl_psaresubmitbmv2l51.apply();
            if (istd.packet_path == PSA_PacketPath_t.NORMAL) {
                tbl_psaresubmitbmv2l53.apply();
            } else if (istd.packet_path == PSA_PacketPath_t.NORMAL_UNICAST) {
                tbl_psaresubmitbmv2l55.apply();
            } else if (istd.packet_path == PSA_PacketPath_t.NORMAL_MULTICAST) {
                tbl_psaresubmitbmv2l57.apply();
            } else if (istd.packet_path == PSA_PacketPath_t.CLONE_I2E) {
                tbl_psaresubmitbmv2l59.apply();
            } else if (istd.packet_path == PSA_PacketPath_t.CLONE_E2E) {
                tbl_psaresubmitbmv2l61.apply();
            } else if (istd.packet_path == PSA_PacketPath_t.RESUBMIT) {
                tbl_psaresubmitbmv2l63.apply();
            } else if (istd.packet_path == PSA_PacketPath_t.RECIRCULATE) {
                tbl_psaresubmitbmv2l65.apply();
            }
        }
    }
}

parser EgressParserImpl(packet_in buffer, out headers_t hdr, inout metadata_t user_meta, in psa_egress_parser_input_metadata_t istd, in empty_metadata_t normal_meta, in empty_metadata_t clone_i2e_meta, in empty_metadata_t clone_e2e_meta) {
    state start {
        buffer.extract < ethernet_t >(hdr.ethernet);
        transition accept;
    }
}

control cEgress(inout headers_t hdr, inout metadata_t user_meta, in psa_egress_input_metadata_t istd, inout psa_egress_output_metadata_t ostd) {
    apply {
    }
}

control IngressDeparserImpl(packet_out buffer, out empty_metadata_t clone_i2e_meta, out empty_metadata_t resubmit_meta, out empty_metadata_t normal_meta, inout headers_t hdr, in metadata_t meta, in psa_ingress_output_metadata_t istd) {
    @hidden action psaresubmitbmv2l140() {
        buffer.emit < ethernet_t >(hdr.ethernet);
        buffer.emit < output_data_t >(hdr.output_data);
    }
    @hidden table tbl_psaresubmitbmv2l140 {
        actions = {
            psaresubmitbmv2l140();
        }
        const default_action = psaresubmitbmv2l140();
    }
    apply {
        tbl_psaresubmitbmv2l140.apply();
    }
}

control EgressDeparserImpl(packet_out buffer, out empty_metadata_t clone_e2e_meta, out empty_metadata_t recirculate_meta, inout headers_t hdr, in metadata_t meta, in psa_egress_output_metadata_t istd, in psa_egress_deparser_input_metadata_t edstd) {
    @hidden action psaresubmitbmv2l140_0() {
        buffer.emit < ethernet_t >(hdr.ethernet);
        buffer.emit < output_data_t >(hdr.output_data);
    }
    @hidden table tbl_psaresubmitbmv2l140_0 {
        actions = {
            psaresubmitbmv2l140_0();
        }
        const default_action = psaresubmitbmv2l140_0();
    }
    apply {
        tbl_psaresubmitbmv2l140_0.apply();
    }
}

IngressPipeline < headers_t, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t >(IngressParserImpl(), cIngress(), IngressDeparserImpl()) ip;
EgressPipeline < headers_t, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t >(EgressParserImpl(), cEgress(), EgressDeparserImpl()) ep;
PSA_Switch < headers_t, metadata_t, headers_t, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t >(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
