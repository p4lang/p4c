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
    ethernet_t tmp_0_eth_hdr;
    bit<128> key_0;
    bit<48> key_1;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_2() {
    }
    @name("ingress.set_valid_action") action set_valid_action() {
        tmp_0_eth_hdr.setValid();
        tmp_0_eth_hdr.dst_addr = 48w1;
        tmp_0_eth_hdr.src_addr = 48w1;
        h.eth_hdr = tmp_0_eth_hdr;
    }
    @name("ingress.simple_table_1") table simple_table {
        key = {
            key_0: exact @name("JGOUaj");
        }
        actions = {
            set_valid_action();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    @name("ingress.simple_table_2") table simple_table_0 {
        key = {
            key_1: exact @name("qkgOtm");
        }
        actions = {
            NoAction_2();
        }
        default_action = NoAction_2();
    }
    @hidden action gauntlet_switch_nested_table_applybmv2l34() {
        key_0 = 128w1;
    }
    @hidden action gauntlet_switch_nested_table_applybmv2l42() {
        key_1 = 48w1;
    }
    @hidden table tbl_gauntlet_switch_nested_table_applybmv2l42 {
        actions = {
            gauntlet_switch_nested_table_applybmv2l42();
        }
        const default_action = gauntlet_switch_nested_table_applybmv2l42();
    }
    @hidden table tbl_gauntlet_switch_nested_table_applybmv2l34 {
        actions = {
            gauntlet_switch_nested_table_applybmv2l34();
        }
        const default_action = gauntlet_switch_nested_table_applybmv2l34();
    }
    apply {
        tbl_gauntlet_switch_nested_table_applybmv2l42.apply();
        switch (simple_table_0.apply().action_run) {
            NoAction_2: {
                tbl_gauntlet_switch_nested_table_applybmv2l34.apply();
                simple_table.apply();
            }
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
