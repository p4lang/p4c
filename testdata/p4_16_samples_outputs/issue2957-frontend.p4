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
    H[2]       h;
}

extern bit<8> extern_call(inout H val);
parser p(packet_in pkt, out Headers hdr) {
    @name("p.tmp_2") bit<3> tmp;
    @name("p.tmp_3") bit<3> tmp_2;
    @name("p.tmp_4") H tmp_3;
    @name("p.tmp_5") bit<8> tmp_4;
    @name("p.val_0") bit<3> val_1;
    @name("p.bound_0") bit<3> bound;
    @name("p.retval") bit<3> retval;
    @name("p.tmp") bit<3> tmp_5;
    state start {
        val_1 = 3w1;
        bound = 3w2;
        if (val_1 < bound) {
            tmp_5 = val_1;
        } else {
            tmp_5 = bound;
        }
        retval = tmp_5;
        tmp = retval;
        tmp_2 = tmp;
        tmp_3 = hdr.h[tmp_2];
        tmp_4 = extern_call(tmp_3);
        hdr.h[tmp_2] = tmp_3;
        transition start_0;
    }
    state start_0 {
        pkt.extract<ethernet_t>(hdr.eth_hdr);
        pkt.extract<H>(hdr.h.next);
        transition accept;
    }
}

control ingress(inout Headers h) {
    apply {
    }
}

parser Parser(packet_in b, out Headers hdr);
control Ingress(inout Headers hdr);
package top(Parser p, Ingress ig);
top(p(), ingress()) main;
