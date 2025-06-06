#include <core.p4>

header Header {
    bit<32> data;
    bit<32> data2;
}

parser p0(packet_in p, out Header h) {
    state start {
        p.extract(h);
        h.data = 8 + h.data2 - 8 - 8 - 2 - 16;
        transition accept;
    }
}

parser proto(packet_in p, out Header h);
package top(proto _p);

top(p0()) main;
