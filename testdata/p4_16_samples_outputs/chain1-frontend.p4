#include <core.p4>

header Header {
    bit<32> data;
}

parser p1(packet_in p, out Header h) {
    bit<1> x;
    state start {
        transition select(x) {
            1w0: chain1;
            1w1: chain2;
        }
    }
    state chain1 {
        p.extract<Header>(h);
        transition endchain;
    }
    state chain2 {
        p.extract<Header>(h);
        transition endchain;
    }
    state endchain {
        transition accept;
    }
}

parser proto(packet_in p, out Header h);
package top(proto _p);
top(p1()) main;

