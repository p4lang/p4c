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
    action exit_action() {
        exit;
    }

    table tbl {
        key = {
            h.eth_hdr.src_addr: exact @name("fbgPij") ;
        }
        actions = {
            exit_action();
        }
    }
    apply {
        h.eth_hdr.eth_type = (h.eth_hdr.eth_type == 4 ? (tbl.apply().hit ? 16w1 : 16w2) : 16w3);
        exit;

    }
}

parser Parser(packet_in b, out Headers hdr);
control Ingress(inout Headers hdr);
package top(Parser p, Ingress ig);
top(p(), ingress()) main;

