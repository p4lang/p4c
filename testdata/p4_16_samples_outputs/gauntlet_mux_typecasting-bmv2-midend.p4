#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

struct Headers {
    ethernet_t eth_hdr;
}

struct Meta {
}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        pkt.extract<ethernet_t>(hdr.eth_hdr);
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @name("ingress.tmp") bit<3> tmp;
    @hidden action gauntlet_mux_typecastingbmv2l29() {
        tmp = 3w2;
    }
    @hidden action gauntlet_mux_typecastingbmv2l29_0() {
        tmp = 3w1;
    }
    @hidden action gauntlet_mux_typecastingbmv2l29_1() {
        h.eth_hdr.eth_type = (bit<16>)-tmp;
    }
    @hidden table tbl_gauntlet_mux_typecastingbmv2l29 {
        actions = {
            gauntlet_mux_typecastingbmv2l29();
        }
        const default_action = gauntlet_mux_typecastingbmv2l29();
    }
    @hidden table tbl_gauntlet_mux_typecastingbmv2l29_0 {
        actions = {
            gauntlet_mux_typecastingbmv2l29_0();
        }
        const default_action = gauntlet_mux_typecastingbmv2l29_0();
    }
    @hidden table tbl_gauntlet_mux_typecastingbmv2l29_1 {
        actions = {
            gauntlet_mux_typecastingbmv2l29_1();
        }
        const default_action = gauntlet_mux_typecastingbmv2l29_1();
    }
    apply {
        if (h.eth_hdr.src_addr == 48w1) {
            tbl_gauntlet_mux_typecastingbmv2l29.apply();
        } else {
            tbl_gauntlet_mux_typecastingbmv2l29_0.apply();
        }
        tbl_gauntlet_mux_typecastingbmv2l29_1.apply();
    }
}

control vrfy(inout Headers h, inout Meta m) {
    apply {
    }
}

control update(inout Headers h, inout Meta m) {
    apply {
    }
}

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {
    }
}

control deparser(packet_out b, in Headers h) {
    apply {
        b.emit<ethernet_t>(h.eth_hdr);
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
