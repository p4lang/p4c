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
    @name("ingress.tmp") bool tmp;
    @name("ingress.tmp_0") bool tmp_0;
    @name("ingress.tmp_1") bit<16> tmp_1;
    @name(".exit_action") action exit_action_0() {
        hasExited = true;
    }
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("ingress.simple_table_2") table simple_table {
        key = {
            h.eth_hdr.src_addr: exact @name("key");
        }
        actions = {
            exit_action_0();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    @hidden action gauntlet_exit_combination_19bmv2l43() {
        tmp = true;
    }
    @hidden action act() {
        tmp_0 = true;
    }
    @hidden action act_0() {
        tmp_0 = false;
    }
    @hidden action gauntlet_exit_combination_19bmv2l43_0() {
        tmp = tmp_0;
    }
    @hidden action act_1() {
        hasExited = false;
    }
    @hidden action gauntlet_exit_combination_19bmv2l43_1() {
        tmp_1 = 16w2;
    }
    @hidden action gauntlet_exit_combination_19bmv2l43_2() {
        tmp_1 = 16w3;
    }
    @hidden action gauntlet_exit_combination_19bmv2l44() {
        h.eth_hdr.eth_type = tmp_1;
    }
    @hidden table tbl_act {
        actions = {
            act_1();
        }
        const default_action = act_1();
    }
    @hidden table tbl_gauntlet_exit_combination_19bmv2l43 {
        actions = {
            gauntlet_exit_combination_19bmv2l43();
        }
        const default_action = gauntlet_exit_combination_19bmv2l43();
    }
    @hidden table tbl_act_0 {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_act_1 {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    @hidden table tbl_gauntlet_exit_combination_19bmv2l43_0 {
        actions = {
            gauntlet_exit_combination_19bmv2l43_0();
        }
        const default_action = gauntlet_exit_combination_19bmv2l43_0();
    }
    @hidden table tbl_gauntlet_exit_combination_19bmv2l43_1 {
        actions = {
            gauntlet_exit_combination_19bmv2l43_1();
        }
        const default_action = gauntlet_exit_combination_19bmv2l43_1();
    }
    @hidden table tbl_gauntlet_exit_combination_19bmv2l43_2 {
        actions = {
            gauntlet_exit_combination_19bmv2l43_2();
        }
        const default_action = gauntlet_exit_combination_19bmv2l43_2();
    }
    @hidden table tbl_gauntlet_exit_combination_19bmv2l44 {
        actions = {
            gauntlet_exit_combination_19bmv2l44();
        }
        const default_action = gauntlet_exit_combination_19bmv2l44();
    }
    apply {
        tbl_act.apply();
        if (h.eth_hdr.eth_type == 16w1) {
            tbl_gauntlet_exit_combination_19bmv2l43.apply();
        } else {
            if (simple_table.apply().hit) {
                tbl_act_0.apply();
            } else {
                tbl_act_1.apply();
            }
            if (hasExited) {
                ;
            } else {
                tbl_gauntlet_exit_combination_19bmv2l43_0.apply();
            }
        }
        if (hasExited) {
            ;
        } else {
            if (tmp) {
                tbl_gauntlet_exit_combination_19bmv2l43_1.apply();
            } else {
                tbl_gauntlet_exit_combination_19bmv2l43_2.apply();
            }
            tbl_gauntlet_exit_combination_19bmv2l44.apply();
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
