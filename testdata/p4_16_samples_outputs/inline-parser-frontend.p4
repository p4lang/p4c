#include <core.p4>

header Header {
    bit<32> data;
}

parser p1(packet_in p, out Header[2] h) {
    Header h_0;
    state start {
        h_0.setInvalid();
        transition p0_start;
    }
    state p0_start {
        p.extract<Header>(h_0);
        transition start_0;
    }
    state start_0 {
        h[0] = h_0;
        h_0.setInvalid();
        transition p0_start_0;
    }
    state p0_start_0 {
        p.extract<Header>(h_0);
        transition start_1;
    }
    state start_1 {
        h[1] = h_0;
        transition accept;
    }
}

parser proto(packet_in p, out Header[2] h);
package top(proto _p);
top(p1()) main;

