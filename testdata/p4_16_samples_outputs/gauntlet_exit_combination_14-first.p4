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
        transition parse_hdrs;
    }
    state parse_hdrs {
        pkt.extract<ethernet_t>(hdr.eth_hdr);
        transition accept;
    }
}

control ingress(inout Headers h) {
    action exit_action() {
        exit;
    }
    table simple_table {
        key = {
            128w1: exact @name("key") ;
        }
        actions = {
            exit_action();
            @defaultonly NoAction();
        }
        default_action = NoAction();
    }
    apply {
        switch (simple_table.apply().action_run) {
            exit_action: {
                h.eth_hdr.eth_type = 16w1;
                exit;
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

