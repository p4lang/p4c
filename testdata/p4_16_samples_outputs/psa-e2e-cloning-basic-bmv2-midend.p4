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
    @noWarn("unused") @name(".send_to_port") action send_to_port_1() {
        ostd.drop = false;
        ostd.multicast_group = 32w0;
        ostd.egress_port = 32w0xfffffffa;
    }
    @noWarn("unused") @name(".send_to_port") action send_to_port_2() {
        ostd.drop = false;
        ostd.multicast_group = 32w0;
        ostd.egress_port = (bit<32>)hdr.ethernet.dstAddr;
    }
    @hidden table tbl_send_to_port {
        actions = {
            send_to_port_1();
        }
        const default_action = send_to_port_1();
    }
    @hidden table tbl_send_to_port_0 {
        actions = {
            send_to_port_2();
        }
        const default_action = send_to_port_2();
    }
    apply {
        if (hdr.ethernet.dstAddr == 48w8 && istd.packet_path != PSA_PacketPath_t.RECIRCULATE) {
            tbl_send_to_port.apply();
        } else {
            tbl_send_to_port_0.apply();
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
    @noWarn("unused") @name(".egress_drop") action egress_drop_0() {
        ostd.drop = true;
    }
    @name("cEgress.clone") action clone_1() {
        ostd.clone = true;
        ostd.clone_session_id = 16w8;
    }
    @hidden action psae2ecloningbasicbmv2l93() {
        hdr.ethernet.etherType = 16w0xface;
    }
    @hidden action psae2ecloningbasicbmv2l99() {
        ostd.clone_session_id = 16w9;
    }
    @hidden action psae2ecloningbasicbmv2l102() {
        hdr.ethernet.srcAddr = 48w0xbeef;
        ostd.clone_session_id = 16w10;
    }
    @hidden action psae2ecloningbasicbmv2l106() {
        ostd.clone_session_id = 16w11;
    }
    @hidden action psae2ecloningbasicbmv2l111() {
        hdr.ethernet.srcAddr = 48w0xcafe;
    }
    @hidden table tbl_psae2ecloningbasicbmv2l93 {
        actions = {
            psae2ecloningbasicbmv2l93();
        }
        const default_action = psae2ecloningbasicbmv2l93();
    }
    @hidden table tbl_clone {
        actions = {
            clone_1();
        }
        const default_action = clone_1();
    }
    @hidden table tbl_egress_drop {
        actions = {
            egress_drop_0();
        }
        const default_action = egress_drop_0();
    }
    @hidden table tbl_psae2ecloningbasicbmv2l99 {
        actions = {
            psae2ecloningbasicbmv2l99();
        }
        const default_action = psae2ecloningbasicbmv2l99();
    }
    @hidden table tbl_psae2ecloningbasicbmv2l102 {
        actions = {
            psae2ecloningbasicbmv2l102();
        }
        const default_action = psae2ecloningbasicbmv2l102();
    }
    @hidden table tbl_psae2ecloningbasicbmv2l106 {
        actions = {
            psae2ecloningbasicbmv2l106();
        }
        const default_action = psae2ecloningbasicbmv2l106();
    }
    @hidden table tbl_psae2ecloningbasicbmv2l111 {
        actions = {
            psae2ecloningbasicbmv2l111();
        }
        const default_action = psae2ecloningbasicbmv2l111();
    }
    apply {
        if (istd.packet_path == PSA_PacketPath_t.CLONE_E2E) {
            tbl_psae2ecloningbasicbmv2l93.apply();
        } else {
            tbl_clone.apply();
            if (hdr.ethernet.dstAddr == 48w9) {
                tbl_egress_drop.apply();
                tbl_psae2ecloningbasicbmv2l99.apply();
            }
            if (istd.egress_port == 32w0xfffffffa) {
                tbl_psae2ecloningbasicbmv2l102.apply();
            } else {
                if (hdr.ethernet.dstAddr == 48w8) {
                    tbl_psae2ecloningbasicbmv2l106.apply();
                }
                tbl_psae2ecloningbasicbmv2l111.apply();
            }
        }
    }
}

control IngressDeparserImpl(packet_out buffer, out empty_metadata_t clone_i2e_meta, out empty_metadata_t resubmit_meta, out empty_metadata_t normal_meta, inout headers_t hdr, in metadata_t meta, in psa_ingress_output_metadata_t istd) {
    @hidden action psae2ecloningbasicbmv2l121() {
        buffer.emit<ethernet_t>(hdr.ethernet);
    }
    @hidden table tbl_psae2ecloningbasicbmv2l121 {
        actions = {
            psae2ecloningbasicbmv2l121();
        }
        const default_action = psae2ecloningbasicbmv2l121();
    }
    apply {
        tbl_psae2ecloningbasicbmv2l121.apply();
    }
}

control EgressDeparserImpl(packet_out buffer, out empty_metadata_t clone_e2e_meta, out empty_metadata_t recirculate_meta, inout headers_t hdr, in metadata_t meta, in psa_egress_output_metadata_t istd, in psa_egress_deparser_input_metadata_t edstd) {
    @hidden action psae2ecloningbasicbmv2l121_0() {
        buffer.emit<ethernet_t>(hdr.ethernet);
    }
    @hidden table tbl_psae2ecloningbasicbmv2l121_0 {
        actions = {
            psae2ecloningbasicbmv2l121_0();
        }
        const default_action = psae2ecloningbasicbmv2l121_0();
    }
    apply {
        tbl_psae2ecloningbasicbmv2l121_0.apply();
    }
}

IngressPipeline<headers_t, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(IngressParserImpl(), cIngress(), IngressDeparserImpl()) ip;
EgressPipeline<headers_t, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(EgressParserImpl(), cEgress(), EgressDeparserImpl()) ep;
PSA_Switch<headers_t, metadata_t, headers_t, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
