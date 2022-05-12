#include <core.p4>

header Header {
    bit<32> data;
    bit<32> data2;
}

parser p0(packet_in p, out Header h) {
    state start {
        p.extract<Header>(h);
        h.data = h.data2 - 32w8 - 32w8 - 32w2 - 32w16;
        transition accept;
    }
}

parser proto(packet_in p, out Header h);
package top(proto _p);
top(p0()) main;

