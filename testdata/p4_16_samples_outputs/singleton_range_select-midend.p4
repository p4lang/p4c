#include <core.p4>

header h_t {
    bit<32> data;
}

parser p0(packet_in p, out h_t h) {
    state start {
        p.extract<h_t>(h);
        transition select(h.data) {
            32w5: accept;
            default: reject;
        }
    }
}

parser proto(packet_in p, out h_t h);
package top(proto _p);
top(p0()) main;
