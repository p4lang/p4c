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
    bit<64> key_0;
    @noWarn("unused") @name(".NoAction") action NoAction_0() {
    }
    @name("ingress.exit_action") action exit_action() {
        h.eth_hdr.src_addr = 48w2;
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
    @hidden action gauntlet_exit_combination_10l30() {
        hasExited = false;
        key_0 = 64w100;
    }
    @hidden table tbl_gauntlet_exit_combination_10l30 {
        actions = {
            gauntlet_exit_combination_10l30();
        }
        const default_action = gauntlet_exit_combination_10l30();
    }
    apply {
        tbl_gauntlet_exit_combination_10l30.apply();
        simple_table_0.apply();
    }
}

parser Parser(packet_in b, out Headers hdr);
control Ingress(inout Headers hdr);
package top(Parser p, Ingress ig);
top(p(), ingress()) main;

