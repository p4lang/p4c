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
    @noWarn("unused") @name(".NoAction") action NoAction_0() {
    }
    @name("ingress.simple_action") action simple_action() {
        h.eth_hdr.dst_addr = 48w1;
        hasExited = true;
    }
    @name("ingress.simple_table") table simple_table_0 {
        key = {
            h.eth_hdr.eth_type: exact @name("key") ;
        }
        actions = {
            simple_action();
            @defaultonly NoAction_0();
        }
        default_action = NoAction_0();
    }
    @hidden action gauntlet_exit_combination_23l39() {
        hasExited = true;
    }
    @hidden action act() {
        hasExited = false;
    }
    @hidden action gauntlet_exit_combination_23l42() {
        h.eth_hdr.dst_addr = h.eth_hdr.src_addr + h.eth_hdr.dst_addr;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_gauntlet_exit_combination_23l39 {
        actions = {
            gauntlet_exit_combination_23l39();
        }
        const default_action = gauntlet_exit_combination_23l39();
    }
    @hidden table tbl_gauntlet_exit_combination_23l42 {
        actions = {
            gauntlet_exit_combination_23l42();
        }
        const default_action = gauntlet_exit_combination_23l42();
    }
    apply {
        tbl_act.apply();
        switch (simple_table_0.apply().action_run) {
            simple_action: {
                if (!hasExited) {
                    tbl_gauntlet_exit_combination_23l39.apply();
                }
            }
            default: {
            }
        }
        if (!hasExited) {
            tbl_gauntlet_exit_combination_23l42.apply();
        }
    }
}

parser Parser(packet_in b, out Headers hdr);
control Ingress(inout Headers hdr);
package top(Parser p, Ingress ig);
top(p(), ingress()) main;

