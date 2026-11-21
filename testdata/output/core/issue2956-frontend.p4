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
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("ingress.simple_assign") action simple_assign() {
    }
    @name("ingress.simple_assign") action simple_assign_1() {
    }
    @name("ingress.simple_assign") action simple_assign_2() {
        h.eth_hdr.eth_type = 16w1;
    }
    @name("ingress.dummy_table") table dummy_table_0 {
        key = {
            h.eth_hdr.src_addr: exact @name("key");
        }
        actions = {
            NoAction_1();
            simple_assign();
        }
        default_action = NoAction_1();
    }
    apply {
        switch (dummy_table_0.apply().action_run) {
            simple_assign: {
                simple_assign_1();
            }
            NoAction_1: {
            }
        }
        simple_assign_2();
    }
}

parser Parser(packet_in b, out Headers hdr);
control Ingress(inout Headers hdr);
package top(Parser p, Ingress ig);
top(p(), ingress()) main;
