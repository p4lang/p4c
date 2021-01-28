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
    H[3]       h;
    bit<1>     idx;
}

parser p(packet_in pkt, out Headers hdr) {
    state start {
        transition accept;
    }
}

control ingress(inout Headers h) {
    @name("ingress.tmp") bit<1> tmp;
    @name("ingress.tmp_0") bit<1> tmp_0;
    @name("ingress.tmp_1") bit<8> tmp_1;
    @name("ingress.tmp_2") bit<1> tmp_2;
    @name("ingress.tmp_3") bit<1> tmp_3;
    apply {
        {
            @name("ingress.val_0") bit<1> val_0 = h.idx;
            @name("ingress.hasReturned") bool hasReturned = false;
            @name("ingress.retval") bit<1> retval;
            hasReturned = true;
            retval = val_0;
            tmp = retval;
        }
        tmp_0 = tmp;
        tmp_1 = h.h[tmp_0].a;
        {
            @name("ingress.val_1") bit<8> val_1 = tmp_1;
            @name("ingress.hasReturned_0") bool hasReturned_0 = false;
            @name("ingress.retval_0") bit<1> retval_0;
            hasReturned_0 = true;
            retval_0 = 1w0;
            tmp_1 = val_1;
            tmp_2 = retval_0;
        }
        h.h[tmp_0].a = tmp_1;
        tmp_3 = tmp_2;
        h.h[tmp_3].a = 8w1;
    }
}

parser Parser(packet_in b, out Headers hdr);
control Ingress(inout Headers hdr);
package top(Parser p, Ingress ig);
top(p(), ingress()) main;

