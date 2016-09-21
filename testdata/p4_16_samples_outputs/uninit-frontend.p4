#include <core.p4>

header Header {
    bit<32> data1;
    bit<32> data2;
    bit<32> data3;
}

extern void f(in Header h);
extern bit<32> g(inout bit<32> v, in bit<32> w);
parser p1(packet_in p, out Header h) {
    @name("stack") Header[2] stack_0;
    @name("b") bool b_0;
    bit<32> tmp;
    bit<32> tmp_0;
    bit<32> tmp_1;
    bit<32> tmp_2;
    bit<32> tmp_3;
    bit<32> tmp_4;
    state start {
        h.data1 = 32w0;
        f(h);
        tmp = h.data2;
        tmp_0 = h.data2;
        tmp_1 = h.data2;
        tmp_2 = g(tmp_0, tmp_1);
        h.data2 = tmp_0;
        tmp_3 = tmp_2;
        g(tmp, tmp_3);
        h.data2 = tmp;
        tmp_4 = h.data3 + 32w1;
        h.data2 = tmp_4;
        stack_0[0] = stack_0[1];
        b_0 = stack_0[1].isValid();
        transition accept;
    }
}

parser proto(packet_in p, out Header h);
package top(proto _p);
top(p1()) main;
