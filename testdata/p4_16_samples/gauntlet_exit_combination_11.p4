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

    action simple_action() {
        if (false) {
            return;
        } else {
            exit;
        }
    }
    table simple_table {
        key = {
            64w1: exact @name("key") ;
        }
        actions = {
            simple_action();
        }
    }
    apply {
        switch (simple_table.apply().action_run) {
            simple_action: {
                h.eth_hdr.eth_type = 16w1;

            }
        }

        exit;

    }
}

parser Parser(packet_in b, out Headers hdr);
control Ingress(inout Headers hdr);
package top(Parser p, Ingress ig);
top(p(), ingress()) main;

