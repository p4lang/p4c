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
    @name("ingress.val_2") bit<48> val_2;
    @name("ingress.hasReturned") bool hasReturned;
    @hidden action gauntlet_side_effect_order_5l20() {
        val_2 = 48w3;
        hasReturned = true;
    }
    @hidden action gauntlet_side_effect_order_5l23() {
        val_2 = 48w12;
    }
    @hidden action gauntlet_side_effect_order_5l26() {
        val_2 = 48w1452;
    }
    @hidden action act() {
        hasReturned = false;
    }
    @hidden action gauntlet_side_effect_order_5l29() {
        hasReturned = true;
    }
    @hidden action act_0() {
        h.eth_hdr.src_addr = val_2;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_gauntlet_side_effect_order_5l20 {
        actions = {
            gauntlet_side_effect_order_5l20();
        }
        const default_action = gauntlet_side_effect_order_5l20();
    }
    @hidden table tbl_gauntlet_side_effect_order_5l23 {
        actions = {
            gauntlet_side_effect_order_5l23();
        }
        const default_action = gauntlet_side_effect_order_5l23();
    }
    @hidden table tbl_gauntlet_side_effect_order_5l26 {
        actions = {
            gauntlet_side_effect_order_5l26();
        }
        const default_action = gauntlet_side_effect_order_5l26();
    }
    @hidden table tbl_gauntlet_side_effect_order_5l29 {
        actions = {
            gauntlet_side_effect_order_5l29();
        }
        const default_action = gauntlet_side_effect_order_5l29();
    }
    @hidden table tbl_act_0 {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    apply {
        tbl_act.apply();
        if (h.eth_hdr.src_addr <= 48w100) {
            if (h.eth_hdr.src_addr <= 48w50) {
                tbl_gauntlet_side_effect_order_5l20.apply();
            } else {
                tbl_gauntlet_side_effect_order_5l23.apply();
            }
            if (!hasReturned) {
                if (h.eth_hdr.src_addr <= 48w25) {
                    tbl_gauntlet_side_effect_order_5l26.apply();
                }
            }
        }
        if (!hasReturned) {
            tbl_gauntlet_side_effect_order_5l29.apply();
        }
        tbl_act_0.apply();
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

