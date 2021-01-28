#include <core.p4>

bit<3> bound(in bit<3> val, in bit<3> bound_val) {
    return (val > bound_val ? bound_val : val);
}

header I {
    bit<3>  a;
}

header H {
    bit<32>  a;
}

struct Headers {
    I     i;
    H[2]  h;
}

parser p(packet_in pkt, out Headers hdr) {
    state start {
        transition parse_hdrs;
    }
    state parse_hdrs {
        transition accept;
    }
}

control ingress(inout Headers h) {
    apply {
        h.h[bound(h.i.a, 3w1)].a = 32w1;
    }
}

parser Parser(packet_in b, out Headers hdr);
control Ingress(inout Headers hdr);
package top(Parser p, Ingress ig);
top(p(), ingress()) main;

