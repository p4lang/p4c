#include <core.p4>

header Header {
    bit<32> data;
}

parser p0(packet_in p, out Header h) {
    state start {
        p.extract(h);
        h.data[7:0] = 8;
        h.data[7:0] = 8;
        transition select(h.data[15:8]) {
            0: next;
            default: next2;
        }
    }
    state next {
        h.data = 8;
        h.data = 8;
        transition final;
    }
    state next2 {
        h.data[7:2] = 9;
        h.data[7:2] = h.data[7:2];
        h.data[7:2] = 3;
        transition final;
    }
    state final {
        h.data[15:8] = 6;
        h.data[15:8] = 6;
        h.data[15:8] = 6;
        transition accept;
    }
}

parser proto(packet_in p, out Header h);
package top(proto _p);
top(p0()) main;
