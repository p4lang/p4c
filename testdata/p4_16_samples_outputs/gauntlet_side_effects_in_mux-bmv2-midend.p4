#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header H {
    bit<8> a;
    bit<8> b;
}

struct Headers {
    ethernet_t eth_hdr;
    H          h;
}

struct Meta {
}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        pkt.extract<ethernet_t>(hdr.eth_hdr);
        pkt.extract<H>(hdr.h);
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @name("ingress.hPSe_0") bit<8> hPSe;
    @name("ingress.hPSe_1") bit<8> hPSe_2;
    @hidden action act() {
        h.h.a = hPSe;
    }
    @hidden action act_0() {
        h.h.b = hPSe_2;
    }
    @hidden action gauntlet_side_effects_in_muxbmv2l36() {
        h.eth_hdr.eth_type = 16w2;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_act_0 {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    @hidden table tbl_gauntlet_side_effects_in_muxbmv2l36 {
        actions = {
            gauntlet_side_effects_in_muxbmv2l36();
        }
        const default_action = gauntlet_side_effects_in_muxbmv2l36();
    }
    apply {
        if (h.eth_hdr.src_addr == 48w5) {
            tbl_act.apply();
        } else {
            tbl_act_0.apply();
        }
        tbl_gauntlet_side_effects_in_muxbmv2l36.apply();
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
        b.emit<H>(h.h);
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

