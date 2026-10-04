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
        pkt.extract < ethernet_t >(hdr.eth_hdr);
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
            key_0 : exact @name("Vmhbwk");
        }
        actions = {
            simple_action();
            NoAction_1();
        }
        default_action = NoAction_1();
    }
    @hidden action gauntlet_exit_combination_15bmv2l50() {
        h.eth_hdr.eth_type = 16w1;
        hasExited = true;
    }
    @hidden action gauntlet_exit_combination_15bmv2l54() {
        h.eth_hdr.eth_type = 16w2;
        hasExited = true;
    }
    @hidden action gauntlet_exit_combination_15bmv2l58() {
        h.eth_hdr.eth_type = 16w3;
        hasExited = true;
    }
    @hidden action gauntlet_exit_combination_15bmv2l40() {
        hasExited = false;
        key_0 = 48w1;
    }
    @hidden action gauntlet_exit_combination_15bmv2l62() {
        h.eth_hdr.eth_type = 16w4;
        hasExited = true;
    }
    @hidden table tbl_gauntlet_exit_combination_15bmv2l40 {
        actions = {
            gauntlet_exit_combination_15bmv2l40();
        }
        const default_action = gauntlet_exit_combination_15bmv2l40();
    }
    @hidden table tbl_gauntlet_exit_combination_15bmv2l50 {
        actions = {
            gauntlet_exit_combination_15bmv2l50();
        }
        const default_action = gauntlet_exit_combination_15bmv2l50();
    }
    @hidden table tbl_gauntlet_exit_combination_15bmv2l54 {
        actions = {
            gauntlet_exit_combination_15bmv2l54();
        }
        const default_action = gauntlet_exit_combination_15bmv2l54();
    }
    @hidden table tbl_gauntlet_exit_combination_15bmv2l58 {
        actions = {
            gauntlet_exit_combination_15bmv2l58();
        }
        const default_action = gauntlet_exit_combination_15bmv2l58();
    }
    @hidden table tbl_gauntlet_exit_combination_15bmv2l62 {
        actions = {
            gauntlet_exit_combination_15bmv2l62();
        }
        const default_action = gauntlet_exit_combination_15bmv2l62();
    }
    apply {
        tbl_gauntlet_exit_combination_15bmv2l40.apply();
        switch (simple_table_0.apply().action_run) {
            simple_action : {
            tbl_gauntlet_exit_combination_15bmv2l50.apply();
            }
            NoAction_1 : {
            tbl_gauntlet_exit_combination_15bmv2l54.apply();
            }
            default : {
            tbl_gauntlet_exit_combination_15bmv2l58.apply();
            }
        }
        if (hasExited) {
            ;
        } else {
            tbl_gauntlet_exit_combination_15bmv2l62.apply();
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
        pkt.emit < ethernet_t >(h.eth_hdr);
    }
}

V1Switch < Headers, Meta >(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
