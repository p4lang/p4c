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
    bit<8> key_0;
    @noWarn("unused") @name(".NoAction") action NoAction_0() {
    }
    @name("ingress.dummy_action") action dummy_action() {
    }
    @name("ingress.simple_table") table simple_table_0 {
        key = {
            key_0: exact @name("QIqvRY") ;
        }
        actions = {
            dummy_action();
            @defaultonly NoAction_0();
        }
        default_action = NoAction_0();
    }
    @hidden action gauntlet_exit_combination_16l39() {
        h.eth_hdr.eth_type = 16w1;
        hasReturned = true;
    }
    @hidden action gauntlet_exit_combination_16l28() {
        hasExited = false;
        hasReturned = false;
        key_0 = 8w255;
    }
    @hidden action gauntlet_exit_combination_16l44() {
        hasExited = true;
    }
    @hidden table tbl_gauntlet_exit_combination_16l28 {
        actions = {
            gauntlet_exit_combination_16l28();
        }
        const default_action = gauntlet_exit_combination_16l28();
    }
    @hidden table tbl_gauntlet_exit_combination_16l39 {
        actions = {
            gauntlet_exit_combination_16l39();
        }
        const default_action = gauntlet_exit_combination_16l39();
    }
    @hidden table tbl_gauntlet_exit_combination_16l44 {
        actions = {
            gauntlet_exit_combination_16l44();
        }
        const default_action = gauntlet_exit_combination_16l44();
    }
    apply {
        tbl_gauntlet_exit_combination_16l28.apply();
        switch (simple_table_0.apply().action_run) {
            dummy_action: {
                tbl_gauntlet_exit_combination_16l39.apply();
            }
            default: {
            }
        }
        if (!hasReturned) {
            tbl_gauntlet_exit_combination_16l44.apply();
        }
    }
}

parser Parser(packet_in b, out Headers hdr);
control Ingress(inout Headers hdr);
package top(Parser p, Ingress ig);
top(p(), ingress()) main;

