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
    @name("ingress.val_2") bit<48> val_1;
    @name("ingress.hasReturned") bool hasReturned;
    @hidden action gauntlet_side_effect_order_5bmv2l19() {
        val_1 = 48w3;
        hasReturned = true;
    }
    @hidden action gauntlet_side_effect_order_5bmv2l22() {
        val_1 = 48w12;
    }
    @hidden action gauntlet_side_effect_order_5bmv2l25() {
        val_1 = 48w1452;
    }
    @hidden action act() {
        hasReturned = false;
    }
    @hidden action act_0() {
        h.eth_hdr.src_addr = val_1;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_gauntlet_side_effect_order_5bmv2l19 {
        actions = {
            gauntlet_side_effect_order_5bmv2l19();
        }
        const default_action = gauntlet_side_effect_order_5bmv2l19();
    }
    @hidden table tbl_gauntlet_side_effect_order_5bmv2l22 {
        actions = {
            gauntlet_side_effect_order_5bmv2l22();
        }
        const default_action = gauntlet_side_effect_order_5bmv2l22();
    }
    @hidden table tbl_gauntlet_side_effect_order_5bmv2l25 {
        actions = {
            gauntlet_side_effect_order_5bmv2l25();
        }
        const default_action = gauntlet_side_effect_order_5bmv2l25();
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
                tbl_gauntlet_side_effect_order_5bmv2l19.apply();
            } else {
                tbl_gauntlet_side_effect_order_5bmv2l22.apply();
            }
            if (hasReturned) {
                ;
            } else if (h.eth_hdr.src_addr <= 48w25) {
                tbl_gauntlet_side_effect_order_5bmv2l25.apply();
            }
        }
        tbl_act_0.apply();
    }
}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        pkt.extract<ethernet_t>(hdr.eth_hdr);
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

control deparser(packet_out pkt, in Headers h) {
    apply {
        pkt.emit<ethernet_t>(h.eth_hdr);
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
