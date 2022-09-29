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
    @name("p.tmp_4") H tmp_6;
    @name("p.val_0") bit<3> val_0;
    @name("p.bound_0") bit<3> bound_0;
    @name("p.tmp") bit<3> tmp_8;
    state start {
        val_0 = 3w1;
        bound_0 = 3w2;
        transition start_true;
    }
    state start_true {
        tmp_8 = val_0;
        transition start_join;
    }
    state start_false {
        tmp_8 = bound_0;
        transition start_join;
    }
    state start_join {
        tmp_6 = hdr.h[tmp_8];
        extern_call(tmp_6);
        hdr.h[tmp_8] = tmp_6;
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

