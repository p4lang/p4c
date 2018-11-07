#include <core.p4>

header Header {
    bit<32> data;
}

parser p1(packet_in p, out Header h) {
    bit<1> x_0;
    state start {
        transition select(x_0) {
            1w0: chain1;
            1w1: chain2;
            default: noMatch;
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
    state noMatch {
        verify(false, error.NoMatch);
        transition reject;
    }
}

parser proto(packet_in p, out Header h);
package top(proto _p);
top(p1()) main;

