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
    @noWarn("unused") @name(".NoAction") action NoAction_0() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_3() {
    }
    @name("ingress.simple_table_1") table simple_table {
        key = {
            48w1: exact @name("KOXpQP") ;
        }
        actions = {
            @defaultonly NoAction_0();
        }
        default_action = NoAction_0();
    }
    @name("ingress.simple_table_2") table simple_table_0 {
        key = {
            (simple_table.apply().hit ? 8w1 : 8w2): exact @name("key") ;
        }
        actions = {
            @defaultonly NoAction_3();
        }
        default_action = NoAction_3();
    }
    apply {
        if (simple_table_0.apply().hit) {
            h.eth_hdr.dst_addr = 48w1;
        }
    }
}

parser Parser(packet_in b, out Headers hdr);
control Ingress(inout Headers hdr);
package top(Parser p, Ingress ig);
top(p(), ingress()) main;

