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
        pkt.extract(hdr.eth_hdr);
        transition accept;
    }
}

control ingress(inout Headers h) {

    action dummy_action() {
    }
    table simple_table {
        key = {
            8w255: exact @name("QIqvRY") ;
        }
        actions = {
            dummy_action();
        }
    }
    apply {
        if (false) {
        } else {
            switch (simple_table.apply().action_run) {
                dummy_action: {
                    h.eth_hdr.eth_type = 1;
                    return;
                }
            }
        }
        exit;

    }
}

parser Parser(packet_in b, out Headers hdr);
control Ingress(inout Headers hdr);
package top(Parser p, Ingress ig);
top(p(), ingress()) main;

