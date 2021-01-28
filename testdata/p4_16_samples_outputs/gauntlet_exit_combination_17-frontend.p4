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
    @name("ingress.dummy_action") action dummy_action() {
    }
    @name("ingress.dummy_action") action dummy_action_2() {
    }
    @name("ingress.simple_table_1") table simple_table {
        key = {
            48w1: exact @name("key") ;
        }
        actions = {
            dummy_action();
            @defaultonly NoAction_0();
        }
        default_action = NoAction_0();
    }
    @name("ingress.simple_table_2") table simple_table_0 {
        key = {
            48w1: exact @name("key") ;
        }
        actions = {
            dummy_action_2();
            @defaultonly NoAction_3();
        }
        default_action = NoAction_3();
    }
    apply {
        @name("ingress.hasReturned") bool hasReturned = false;
        switch (simple_table.apply().action_run) {
            dummy_action: {
                switch (simple_table_0.apply().action_run) {
                    dummy_action_2: {
                        h.eth_hdr.src_addr = 48w4;
                        hasReturned = true;
                    }
                    default: {
                    }
                }
            }
            default: {
            }
        }
        if (!hasReturned) {
            exit;
        }
    }
}

parser Parser(packet_in b, out Headers hdr);
control Ingress(inout Headers hdr);
package top(Parser p, Ingress ig);
top(p(), ingress()) main;

