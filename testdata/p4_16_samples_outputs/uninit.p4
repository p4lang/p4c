#include <core.p4>

header Header {
    bit<32> data1;
    bit<32> data2;
}

extern void f(in Header h);
parser p1(packet_in p, out Header h) {
    state start {
        h.data1 = 0;
        f(h);
        transition next;
    }
    state next {
        h.data2 = h.data2 + 1;
        transition accept;
    }
}

parser proto(packet_in p, out Header h);
package top(proto _p);
top(p1()) main;
