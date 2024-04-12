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

parser IngressParserImpl(packet_in pkt, out headers_t hdr, inout metadata_t user_meta, in psa_ingress_parser_input_metadata_t istd, in empty_metadata_t resubmit_meta, in empty_metadata_t recirculate_meta) {
    state start {
        pkt.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
}

control cIngress(inout headers_t hdr, inout metadata_t user_meta, in psa_ingress_input_metadata_t istd, inout psa_ingress_output_metadata_t ostd) {
    @noWarn("unused") @name(".ingress_drop") action ingress_drop_0() {
        ostd.drop = true;
    }
    @noWarn("unused") @name(".send_to_port") action send_to_port_0() {
        ostd.drop = false;
        ostd.multicast_group = 32w0;
        ostd.egress_port = (bit<32>)hdr.ethernet.dstAddr;
    }
    @name("cIngress.clone") action clone_1() {
        ostd.clone = true;
        ostd.clone_session_id = 16w8;
    }
    @hidden action psai2ecloningbasicbmv2l70() {
        hdr.ethernet.srcAddr = 48w0xcafe;
    }
    @hidden table tbl_clone {
        actions = {
            clone_1();
        }
        const default_action = clone_1();
    }
    @hidden table tbl_ingress_drop {
        actions = {
            ingress_drop_0();
        }
        const default_action = ingress_drop_0();
    }
    @hidden table tbl_psai2ecloningbasicbmv2l70 {
        actions = {
            psai2ecloningbasicbmv2l70();
        }
        const default_action = psai2ecloningbasicbmv2l70();
    }
    @hidden table tbl_send_to_port {
        actions = {
            send_to_port_0();
        }
        const default_action = send_to_port_0();
    }
    apply {
        tbl_clone.apply();
        if (hdr.ethernet.dstAddr == 48w9) {
            tbl_ingress_drop.apply();
        } else {
            tbl_psai2ecloningbasicbmv2l70.apply();
            tbl_send_to_port.apply();
        }
    }
}

parser EgressParserImpl(packet_in buffer, out headers_t hdr, inout metadata_t user_meta, in psa_egress_parser_input_metadata_t istd, in empty_metadata_t normal_meta, in empty_metadata_t clone_i2e_meta, in empty_metadata_t clone_e2e_meta) {
    state start {
        buffer.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
}

control cEgress(inout headers_t hdr, inout metadata_t user_meta, in psa_egress_input_metadata_t istd, inout psa_egress_output_metadata_t ostd) {
    @hidden action psai2ecloningbasicbmv2l99() {
        hdr.ethernet.etherType = 16w0xface;
    }
    @hidden table tbl_psai2ecloningbasicbmv2l99 {
        actions = {
            psai2ecloningbasicbmv2l99();
        }
        const default_action = psai2ecloningbasicbmv2l99();
    }
    apply {
        if (istd.packet_path == PSA_PacketPath_t.CLONE_I2E) {
            tbl_psai2ecloningbasicbmv2l99.apply();
        }
    }
}

control IngressDeparserImpl(packet_out buffer, out empty_metadata_t clone_i2e_meta, out empty_metadata_t resubmit_meta, out empty_metadata_t normal_meta, inout headers_t hdr, in metadata_t meta, in psa_ingress_output_metadata_t istd) {
    @hidden action psai2ecloningbasicbmv2l108() {
        buffer.emit<ethernet_t>(hdr.ethernet);
    }
    @hidden table tbl_psai2ecloningbasicbmv2l108 {
        actions = {
            psai2ecloningbasicbmv2l108();
        }
        const default_action = psai2ecloningbasicbmv2l108();
    }
    apply {
        tbl_psai2ecloningbasicbmv2l108.apply();
    }
}

control EgressDeparserImpl(packet_out buffer, out empty_metadata_t clone_e2e_meta, out empty_metadata_t recirculate_meta, inout headers_t hdr, in metadata_t meta, in psa_egress_output_metadata_t istd, in psa_egress_deparser_input_metadata_t edstd) {
    @hidden action psai2ecloningbasicbmv2l108_0() {
        buffer.emit<ethernet_t>(hdr.ethernet);
    }
    @hidden table tbl_psai2ecloningbasicbmv2l108_0 {
        actions = {
            psai2ecloningbasicbmv2l108_0();
        }
        const default_action = psai2ecloningbasicbmv2l108_0();
    }
    apply {
        tbl_psai2ecloningbasicbmv2l108_0.apply();
    }
}

IngressPipeline<headers_t, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(IngressParserImpl(), cIngress(), IngressDeparserImpl()) ip;
EgressPipeline<headers_t, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(EgressParserImpl(), cEgress(), EgressDeparserImpl()) ep;
PSA_Switch<headers_t, metadata_t, headers_t, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
