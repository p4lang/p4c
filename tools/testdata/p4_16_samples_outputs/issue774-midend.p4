#include <core.p4>

header Header {
    bit<32> data;
}

parser p0(packet_in p, out Header h) {
    Header arg;
    state start {
        p.extract<Header>(arg);
        p.extract<Header>(h);
        transition accept;
    }
}

parser proto(packet_in p, out Header h);
package top(proto p);
top(p0()) main;

