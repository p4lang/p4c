#include <core.p4>

header Header {
    bit<32> data1;
    bit<32> data2;
    bit<32> data3;
}

extern void f(in Header h);
extern bit<32> g(inout bit<32> v, in bit<32> w);
parser p1(packet_in p, out Header h) {
    Header[2] stack;
    bool b;
    state start {
        h.data1 = 32w0;
        f(h);
        g(h.data2, g(h.data2, h.data2));
        transition next;
    }
    state next {
        h.data2 = h.data3 + 32w1;
        stack[0] = stack[1];
        b = stack[1].isValid();
        transition accept;
    }
}

parser proto(packet_in p, out Header h);
package top(proto _p);
top(p1()) main;
