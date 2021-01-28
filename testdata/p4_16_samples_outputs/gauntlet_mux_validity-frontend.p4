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
    H          h;
}

parser p(packet_in pkt, out Headers hdr) {
    state start {
        pkt.extract<ethernet_t>(hdr.eth_hdr);
        pkt.extract<H>(hdr.h);
        transition accept;
    }
}

control ingress(inout Headers h) {
    @name("ingress.b") bool b_0;
    @name("ingress.tmp") bit<16> tmp;
    @name("ingress.tmp_0") bit<16> tmp_0;
    @name("ingress.dummy") action dummy_2() {
        if (b_0) {
            {
                @name("ingress.dummy_1") H dummy_1 = h.h;
                @name("ingress.hasReturned") bool hasReturned = false;
                @name("ingress.retval") bit<16> retval;
                hasReturned = true;
                retval = 16w1;
                h.h = dummy_1;
                tmp_0 = retval;
            }
            tmp = tmp_0;
        } else {
            tmp = 16w1;
        }
        h.eth_hdr.eth_type = tmp;
    }
    apply {
        b_0 = false;
        dummy_2();
    }
}

parser Parser(packet_in b, out Headers hdr);
control Ingress(inout Headers hdr);
package top(Parser p, Ingress ig);
top(p(), ingress()) main;

