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
    bit<48> key_0;
    @noWarn("unused") @name(".NoAction") action NoAction_0() {
    }
    @name("ingress.dummy") action dummy() {
    }
    @name("ingress.simple_table") table simple_table_0 {
        key = {
            key_0: exact @name("key") ;
        }
        actions = {
            dummy();
            @defaultonly NoAction_0();
        }
        default_action = NoAction_0();
    }
    @hidden action gauntlet_exit_combination_2l36() {
        h.eth_hdr.src_addr = 48w1;
        hasExited = true;
    }
    @hidden action gauntlet_exit_combination_2l27() {
        hasExited = false;
        key_0 = 48w100;
    }
    @hidden action gauntlet_exit_combination_2l41() {
        h.eth_hdr.dst_addr = 48w5;
    }
    @hidden table tbl_gauntlet_exit_combination_2l27 {
        actions = {
            gauntlet_exit_combination_2l27();
        }
        const default_action = gauntlet_exit_combination_2l27();
    }
    @hidden table tbl_gauntlet_exit_combination_2l36 {
        actions = {
            gauntlet_exit_combination_2l36();
        }
        const default_action = gauntlet_exit_combination_2l36();
    }
    @hidden table tbl_gauntlet_exit_combination_2l41 {
        actions = {
            gauntlet_exit_combination_2l41();
        }
        const default_action = gauntlet_exit_combination_2l41();
    }
    apply {
        tbl_gauntlet_exit_combination_2l27.apply();
        switch (simple_table_0.apply().action_run) {
            dummy: {
                tbl_gauntlet_exit_combination_2l36.apply();
            }
            default: {
            }
        }
        if (!hasExited) {
            tbl_gauntlet_exit_combination_2l41.apply();
        }
    }
}

parser Parser(packet_in b, out Headers hdr);
control Ingress(inout Headers hdr);
package top(Parser p, Ingress ig);
top(p(), ingress()) main;

