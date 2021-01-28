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
    bit<128> key_0;
    @noWarn("unused") @name(".NoAction") action NoAction_0() {
    }
    @name("ingress.exit_action") action exit_action() {
        hasExited = true;
    }
    @name("ingress.simple_table") table simple_table_0 {
        key = {
            key_0: exact @name("key") ;
        }
        actions = {
            exit_action();
            @defaultonly NoAction_0();
        }
        default_action = NoAction_0();
    }
    @hidden action gauntlet_exit_combination_14l37() {
        h.eth_hdr.eth_type = 16w1;
        hasExited = true;
    }
    @hidden action gauntlet_exit_combination_14l28() {
        hasExited = false;
        key_0 = 128w1;
    }
    @hidden table tbl_gauntlet_exit_combination_14l28 {
        actions = {
            gauntlet_exit_combination_14l28();
        }
        const default_action = gauntlet_exit_combination_14l28();
    }
    @hidden table tbl_gauntlet_exit_combination_14l37 {
        actions = {
            gauntlet_exit_combination_14l37();
        }
        const default_action = gauntlet_exit_combination_14l37();
    }
    apply {
        tbl_gauntlet_exit_combination_14l28.apply();
        switch (simple_table_0.apply().action_run) {
            exit_action: {
                if (!hasExited) {
                    tbl_gauntlet_exit_combination_14l37.apply();
                }
            }
            default: {
            }
        }
    }
}

parser Parser(packet_in b, out Headers hdr);
control Ingress(inout Headers hdr);
package top(Parser p, Ingress ig);
top(p(), ingress()) main;

