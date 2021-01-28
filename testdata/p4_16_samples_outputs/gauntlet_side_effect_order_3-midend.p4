#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header H {
    bit<8> a;
    bit<8> b;
    bit<8> c;
}

struct Headers {
    H h;
}

struct Meta {
    bit<8> tmp;
}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @name("ingress.val_0") bit<8> val_0;
    @name("ingress.val_1") bit<8> val_1;
    @hidden action gauntlet_side_effect_order_3l32() {
        m.tmp = val_1;
        h.h.a = 8w1;
    }
    @hidden table tbl_gauntlet_side_effect_order_3l32 {
        actions = {
            gauntlet_side_effect_order_3l32();
        }
        const default_action = gauntlet_side_effect_order_3l32();
    }
    apply {
        tbl_gauntlet_side_effect_order_3l32.apply();
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

