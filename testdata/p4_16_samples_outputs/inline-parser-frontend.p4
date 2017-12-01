#include <core.p4>

header Header {
    bit<32> data;
}

parser p0(packet_in p, out Header h) {
    state start {
        p.extract<Header>(h);
        transition accept;
    }
}

parser p1(packet_in p, out Header[2] h) {
    @name("p0inst") p0() p0inst_0;
    state start {
        p0inst_0.apply(p, h[0]);
        p0inst_0.apply(p, h[1]);
        transition accept;
    }
}

parser proto(packet_in p, out Header[2] h);
package top(proto _p);
top(p1()) main;

