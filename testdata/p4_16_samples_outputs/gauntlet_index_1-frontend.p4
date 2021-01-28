#include <core.p4>

header I {
    bit<3> a;
}

header H {
    bit<32> a;
}

struct Headers {
    I    i;
    H[2] h;
}

parser p(packet_in pkt, out Headers hdr) {
    state start {
        transition accept;
    }
}

control ingress(inout Headers h) {
    @name("ingress.tmp_0") bit<3> tmp;
    @name("ingress.tmp_1") bit<3> tmp_0;
    apply {
        {
            @name("ingress.val_0") bit<3> val_0 = h.i.a;
            @name("ingress.bound_val_0") bit<3> bound_val_0 = 3w1;
            @name("ingress.hasReturned") bool hasReturned = false;
            @name("ingress.retval") bit<3> retval;
            @name("ingress.tmp") bit<3> tmp_1;
            if (val_0 > bound_val_0) {
                tmp_1 = bound_val_0;
            } else {
                tmp_1 = val_0;
            }
            hasReturned = true;
            retval = tmp_1;
            tmp = retval;
        }
        tmp_0 = tmp;
        h.h[tmp_0].a = 32w1;
    }
}

parser Parser(packet_in b, out Headers hdr);
control Ingress(inout Headers hdr);
package top(Parser p, Ingress ig);
top(p(), ingress()) main;

