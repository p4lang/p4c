#include <core.p4>
header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header H {
    bit<8> a;
}

struct Headers {
    ethernet_t eth_hdr;
    H[2]  h;
}

parser p(packet_in pkt, out Headers hdr) {
    state start {
        transition parse_hdrs;
    }
    state parse_hdrs {
        pkt.extract(hdr.eth_hdr);
        pkt.extract(hdr.h.next);
        pkt.extract(hdr.h.next);
        transition accept;
    }
}

control ingress(inout Headers h) {
    bit<8> dummy_val = h.h[0].a;
    action simple_action() {
        h.h[0].a = dummy_val;
    }
    table simple_table {
        key = {
            32w1          : exact @name("IymcAg") ;
        }
        actions = {
            simple_action();
            NoAction();
        }
    }
    apply {
        simple_action();
        dummy_val = 8w255;
        simple_table.apply();
    }
}

parser Parser(packet_in b, out Headers hdr);
control Ingress(inout Headers hdr);
package top(Parser p, Ingress ig);
top(p(), ingress()) main;

