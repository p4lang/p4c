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
    bool hasExited;
    bit<48> key_0;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("ingress.simple_action") action simple_action() {
        h.eth_hdr.src_addr = 48w1;
    }
    @name("ingress.simple_table") table simple_table_0 {
        key = {
            key_0: exact @name("Vmhbwk");
        }
        actions = {
            simple_action();
            NoAction_1();
        }
        default_action = NoAction_1();
    }
    @hidden action gauntlet_exit_combination_15bmv2l43() {
        h.eth_hdr.eth_type = 16w1;
        hasExited = true;
    }
    @hidden action gauntlet_exit_combination_15bmv2l47() {
        h.eth_hdr.eth_type = 16w2;
        hasExited = true;
    }
    @hidden action gauntlet_exit_combination_15bmv2l51() {
        h.eth_hdr.eth_type = 16w3;
        hasExited = true;
    }
    @hidden action gauntlet_exit_combination_15bmv2l33() {
        hasExited = false;
        key_0 = 48w1;
    }
    @hidden action gauntlet_exit_combination_15bmv2l55() {
        h.eth_hdr.eth_type = 16w4;
        hasExited = true;
    }
    @hidden table tbl_gauntlet_exit_combination_15bmv2l33 {
        actions = {
            gauntlet_exit_combination_15bmv2l33();
        }
        const default_action = gauntlet_exit_combination_15bmv2l33();
    }
    @hidden table tbl_gauntlet_exit_combination_15bmv2l43 {
        actions = {
            gauntlet_exit_combination_15bmv2l43();
        }
        const default_action = gauntlet_exit_combination_15bmv2l43();
    }
    @hidden table tbl_gauntlet_exit_combination_15bmv2l47 {
        actions = {
            gauntlet_exit_combination_15bmv2l47();
        }
        const default_action = gauntlet_exit_combination_15bmv2l47();
    }
    @hidden table tbl_gauntlet_exit_combination_15bmv2l51 {
        actions = {
            gauntlet_exit_combination_15bmv2l51();
        }
        const default_action = gauntlet_exit_combination_15bmv2l51();
    }
    @hidden table tbl_gauntlet_exit_combination_15bmv2l55 {
        actions = {
            gauntlet_exit_combination_15bmv2l55();
        }
        const default_action = gauntlet_exit_combination_15bmv2l55();
    }
    apply {
        tbl_gauntlet_exit_combination_15bmv2l33.apply();
        switch (simple_table_0.apply().action_run) {
            simple_action: {
                tbl_gauntlet_exit_combination_15bmv2l43.apply();
            }
            NoAction_1: {
                tbl_gauntlet_exit_combination_15bmv2l47.apply();
            }
            default: {
                tbl_gauntlet_exit_combination_15bmv2l51.apply();
            }
        }
        if (hasExited) {
            ;
        } else {
            tbl_gauntlet_exit_combination_15bmv2l55.apply();
        }
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
