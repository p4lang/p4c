#include <core.p4>

header Header {
    bit<32> data;
}

parser p1(packet_in p, out Header h) {
    state start {
        p.extract<Header>(h);
        transition accept;
    }
}

parser proto(packet_in p, out Header h);
package top(proto _p);
top(p1()) main;

