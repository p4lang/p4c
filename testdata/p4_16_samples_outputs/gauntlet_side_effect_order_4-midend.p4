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

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @name("ingress.tmp") bit<16> tmp;
    @hidden action gauntlet_side_effect_order_4l18() {
        tmp = 16w0;
    }
    @hidden action gauntlet_side_effect_order_4l18_0() {
        tmp = 16w3;
    }
    @hidden action act() {
        h.eth_hdr.eth_type = tmp;
    }
    @hidden table tbl_gauntlet_side_effect_order_4l18 {
        actions = {
            gauntlet_side_effect_order_4l18();
        }
        const default_action = gauntlet_side_effect_order_4l18();
    }
    @hidden table tbl_gauntlet_side_effect_order_4l18_0 {
        actions = {
            gauntlet_side_effect_order_4l18_0();
        }
        const default_action = gauntlet_side_effect_order_4l18_0();
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        if (h.eth_hdr.eth_type < 16w6) {
            tbl_gauntlet_side_effect_order_4l18.apply();
        } else {
            tbl_gauntlet_side_effect_order_4l18_0.apply();
        }
        tbl_act.apply();
    }
}

parser p(packet_in b, out Headers h, inout Meta m, inout standard_metadata_t sm) {
    state start {
        transition accept;
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
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

