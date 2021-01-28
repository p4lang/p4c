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
    @name("ingress.simple_action") action simple_action() {
        h.eth_hdr.src_addr = 48w1;
    }
    @name("ingress.simple_table") table simple_table_0 {
        key = {
            48w1: exact @name("Vmhbwk") ;
        }
        actions = {
            simple_action();
            @defaultonly NoAction_0();
        }
        default_action = NoAction_0();
    }
    apply {
        @name("ingress.hasReturned") bool hasReturned = false;
        switch (simple_table_0.apply().action_run) {
            simple_action: {
                hasReturned = true;
            }
            default: {
            }
        }
        if (!hasReturned) {
            h.eth_hdr.eth_type = 16w2;
            exit;
        }
    }
}

parser Parser(packet_in b, out Headers hdr);
control Ingress(inout Headers hdr);
package top(Parser p, Ingress ig);
top(p(), ingress()) main;

