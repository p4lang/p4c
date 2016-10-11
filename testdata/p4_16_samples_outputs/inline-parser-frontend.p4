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
    Header tmp;
    Header tmp_0;
    @name("p0inst") p0() p0inst_0;
    state start {
        p0inst_0.apply(p, tmp);
        h[0] = tmp;
        p0inst_0.apply(p, tmp_0);
        h[1] = tmp_0;
        transition accept;
    }
}

parser proto(packet_in p, out Header[2] h);
package top(proto _p);
top(p1()) main;
