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
    @name("ingress.dummy") action dummy() {
    }
    @name("ingress.do_action") action do_action() {
        h.eth_hdr.eth_type = 16w1;
        exit;
    }
    @name("ingress.simple_table") table simple_table_0 {
        key = {
            32w1: exact @name("akSTMF") ;
        }
        actions = {
            dummy();
            NoAction_0();
        }
        default_action = NoAction_0();
    }
    apply {
        @name("ingress.hasReturned") bool hasReturned = false;
        switch (simple_table_0.apply().action_run) {
            dummy: {
            }
            default: {
                hasReturned = true;
            }
        }
        if (!hasReturned) {
            do_action();
        }
    }
}

parser Parser(packet_in b, out Headers hdr);
control Ingress(inout Headers hdr);
package top(Parser p, Ingress ig);
top(p(), ingress()) main;

