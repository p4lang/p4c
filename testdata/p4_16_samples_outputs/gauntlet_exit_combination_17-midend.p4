#include <core.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

struct Headers {
    ethernet_t eth_hdr;
}

parser p(packet_in pkt, out Headers hdr) {
    state start {
        pkt.extract<ethernet_t>(hdr.eth_hdr);
        transition accept;
    }
}

control ingress(inout Headers h) {
    bool hasExited;
    @name("ingress.hasReturned") bool hasReturned;
    bit<48> key_0;
    bit<48> key_1;
    @noWarn("unused") @name(".NoAction") action NoAction_0() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_3() {
    }
    @name("ingress.dummy_action") action dummy_action() {
    }
    @name("ingress.dummy_action") action dummy_action_2() {
    }
    @name("ingress.simple_table_1") table simple_table {
        key = {
            key_0: exact @name("key") ;
        }
        actions = {
            dummy_action();
            @defaultonly NoAction_0();
        }
        default_action = NoAction_0();
    }
    @name("ingress.simple_table_2") table simple_table_0 {
        key = {
            key_1: exact @name("key") ;
        }
        actions = {
            dummy_action_2();
            @defaultonly NoAction_3();
        }
        default_action = NoAction_3();
    }
    @hidden action gauntlet_exit_combination_17l48() {
        h.eth_hdr.src_addr = 48w4;
        hasReturned = true;
    }
    @hidden action gauntlet_exit_combination_17l37() {
        key_1 = 48w1;
    }
    @hidden action gauntlet_exit_combination_17l29() {
        hasExited = false;
        hasReturned = false;
        key_0 = 48w1;
    }
    @hidden action gauntlet_exit_combination_17l54() {
        hasExited = true;
    }
    @hidden table tbl_gauntlet_exit_combination_17l29 {
        actions = {
            gauntlet_exit_combination_17l29();
        }
        const default_action = gauntlet_exit_combination_17l29();
    }
    @hidden table tbl_gauntlet_exit_combination_17l37 {
        actions = {
            gauntlet_exit_combination_17l37();
        }
        const default_action = gauntlet_exit_combination_17l37();
    }
    @hidden table tbl_gauntlet_exit_combination_17l48 {
        actions = {
            gauntlet_exit_combination_17l48();
        }
        const default_action = gauntlet_exit_combination_17l48();
    }
    @hidden table tbl_gauntlet_exit_combination_17l54 {
        actions = {
            gauntlet_exit_combination_17l54();
        }
        const default_action = gauntlet_exit_combination_17l54();
    }
    apply {
        tbl_gauntlet_exit_combination_17l29.apply();
        switch (simple_table.apply().action_run) {
            dummy_action: {
                tbl_gauntlet_exit_combination_17l37.apply();
                switch (simple_table_0.apply().action_run) {
                    dummy_action_2: {
                        tbl_gauntlet_exit_combination_17l48.apply();
                    }
                    default: {
                    }
                }
            }
            default: {
            }
        }
        if (!hasReturned) {
            tbl_gauntlet_exit_combination_17l54.apply();
        }
    }
}

parser Parser(packet_in b, out Headers hdr);
control Ingress(inout Headers hdr);
package top(Parser p, Ingress ig);
top(p(), ingress()) main;

