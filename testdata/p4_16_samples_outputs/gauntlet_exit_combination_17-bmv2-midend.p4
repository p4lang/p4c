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
    @name("ingress.hasReturned") bool hasReturned;
    bit<48> key_0;
    bit<48> key_1;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_2() {
    }
    @name("ingress.dummy_action") action dummy_action() {
    }
    @name("ingress.dummy_action") action dummy_action_1() {
    }
    @name("ingress.simple_table_1") table simple_table {
        key = {
            key_0: exact @name("key");
        }
        actions = {
            dummy_action();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    @name("ingress.simple_table_2") table simple_table_0 {
        key = {
            key_1: exact @name("key");
        }
        actions = {
            dummy_action_1();
            @defaultonly NoAction_2();
        }
        default_action = NoAction_2();
    }
    @hidden action gauntlet_exit_combination_17bmv2l49() {
        h.eth_hdr.src_addr = 48w4;
        hasReturned = true;
    }
    @hidden action gauntlet_exit_combination_17bmv2l38() {
        key_1 = 48w1;
    }
    @hidden action gauntlet_exit_combination_17bmv2l30() {
        hasExited = false;
        hasReturned = false;
        key_0 = 48w1;
    }
    @hidden action gauntlet_exit_combination_17bmv2l55() {
        hasExited = true;
    }
    @hidden table tbl_gauntlet_exit_combination_17bmv2l30 {
        actions = {
            gauntlet_exit_combination_17bmv2l30();
        }
        const default_action = gauntlet_exit_combination_17bmv2l30();
    }
    @hidden table tbl_gauntlet_exit_combination_17bmv2l38 {
        actions = {
            gauntlet_exit_combination_17bmv2l38();
        }
        const default_action = gauntlet_exit_combination_17bmv2l38();
    }
    @hidden table tbl_gauntlet_exit_combination_17bmv2l49 {
        actions = {
            gauntlet_exit_combination_17bmv2l49();
        }
        const default_action = gauntlet_exit_combination_17bmv2l49();
    }
    @hidden table tbl_gauntlet_exit_combination_17bmv2l55 {
        actions = {
            gauntlet_exit_combination_17bmv2l55();
        }
        const default_action = gauntlet_exit_combination_17bmv2l55();
    }
    apply {
        tbl_gauntlet_exit_combination_17bmv2l30.apply();
        switch (simple_table.apply().action_run) {
            dummy_action: {
                tbl_gauntlet_exit_combination_17bmv2l38.apply();
                switch (simple_table_0.apply().action_run) {
                    dummy_action_1: {
                        tbl_gauntlet_exit_combination_17bmv2l49.apply();
                    }
                    default: {
                    }
                }
            }
            default: {
            }
        }
        if (hasReturned) {
            ;
        } else {
            tbl_gauntlet_exit_combination_17bmv2l55.apply();
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
