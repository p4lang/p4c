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
        h.eth_hdr.dst_addr = 1;
        exit;
    }

    table simple_table {
        key = {
            h.eth_hdr.eth_type: exact @name("key") ;
        }
        actions = {
            simple_action();
        }
    }
    apply {
        switch (simple_table.apply().action_run) {
            simple_action: {
                exit;
            }
        }
        h.eth_hdr.dst_addr = h.eth_hdr.src_addr + h.eth_hdr.dst_addr;
    }
}

parser Parser(packet_in b, out Headers hdr);
control Ingress(inout Headers hdr);
package top(Parser p, Ingress ig);
top(p(), ingress()) main;

