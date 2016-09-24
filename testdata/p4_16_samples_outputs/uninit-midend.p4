#include <core.p4>

header Header {
    bit<32> data1;
    bit<32> data2;
    bit<32> data3;
}

extern void f(in Header h);
extern bit<32> g(inout bit<32> v, in bit<32> w);
parser p1(packet_in p, out Header h) {
    @name("stack") Header[2] stack;
    @name("b") bool b;
    @name("tmp") bit<32> tmp_5;
    @name("tmp_0") bit<32> tmp_6;
    @name("tmp_1") bit<32> tmp_7;
    @name("tmp_2") bit<32> tmp_8;
    @name("tmp_3") bit<32> tmp_9;
    @name("tmp_4") bit<32> tmp_10;
    state start {
        h.data1 = 32w0;
        f(h);
        tmp_5 = h.data2;
        tmp_6 = h.data2;
        tmp_7 = h.data2;
        tmp_8 = g(tmp_6, tmp_7);
        h.data2 = tmp_6;
        tmp_9 = tmp_8;
        g(tmp_5, tmp_9);
        h.data2 = tmp_5;
        tmp_10 = h.data3 + 32w1;
        h.data2 = tmp_10;
        stack[0] = stack[1];
        b = stack[1].isValid();
        transition accept;
    }
}

parser proto(packet_in p, out Header h);
package top(proto _p);
top(p1()) main;
