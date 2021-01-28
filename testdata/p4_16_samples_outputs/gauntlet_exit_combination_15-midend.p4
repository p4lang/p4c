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
    @name("ingress.simple_action") action simple_action() {
        h.eth_hdr.src_addr = 48w1;
    }
    @name("ingress.simple_table") table simple_table_0 {
        key = {
            key_0: exact @name("Vmhbwk") ;
        }
        actions = {
            simple_action();
            NoAction_0();
        }
        default_action = NoAction_0();
    }
    @hidden action gauntlet_exit_combination_15l43() {
        h.eth_hdr.eth_type = 16w1;
        hasExited = true;
    }
    @hidden action gauntlet_exit_combination_15l47() {
        h.eth_hdr.eth_type = 16w2;
        hasExited = true;
    }
    @hidden action gauntlet_exit_combination_15l51() {
        h.eth_hdr.eth_type = 16w3;
        hasExited = true;
    }
    @hidden action gauntlet_exit_combination_15l33() {
        hasExited = false;
        key_0 = 48w1;
    }
    @hidden action gauntlet_exit_combination_15l55() {
        h.eth_hdr.eth_type = 16w4;
        hasExited = true;
    }
    @hidden table tbl_gauntlet_exit_combination_15l33 {
        actions = {
            gauntlet_exit_combination_15l33();
        }
        const default_action = gauntlet_exit_combination_15l33();
    }
    @hidden table tbl_gauntlet_exit_combination_15l43 {
        actions = {
            gauntlet_exit_combination_15l43();
        }
        const default_action = gauntlet_exit_combination_15l43();
    }
    @hidden table tbl_gauntlet_exit_combination_15l47 {
        actions = {
            gauntlet_exit_combination_15l47();
        }
        const default_action = gauntlet_exit_combination_15l47();
    }
    @hidden table tbl_gauntlet_exit_combination_15l51 {
        actions = {
            gauntlet_exit_combination_15l51();
        }
        const default_action = gauntlet_exit_combination_15l51();
    }
    @hidden table tbl_gauntlet_exit_combination_15l55 {
        actions = {
            gauntlet_exit_combination_15l55();
        }
        const default_action = gauntlet_exit_combination_15l55();
    }
    apply {
        tbl_gauntlet_exit_combination_15l33.apply();
        switch (simple_table_0.apply().action_run) {
            simple_action: {
                tbl_gauntlet_exit_combination_15l43.apply();
            }
            NoAction_0: {
                tbl_gauntlet_exit_combination_15l47.apply();
            }
            default: {
                tbl_gauntlet_exit_combination_15l51.apply();
            }
        }
        if (!hasExited) {
            tbl_gauntlet_exit_combination_15l55.apply();
        }
    }
}

parser Parser(packet_in b, out Headers hdr);
control Ingress(inout Headers hdr);
package top(Parser p, Ingress ig);
top(p(), ingress()) main;

