#include <core.p4>

header Header {
    bit<32> data;
}

struct Tst {
    bit<32> data;
}

parser p0(packet_in p, out Header h, out Tst t) {
    bool b = true;
    state start {
        p.extract(h);
        p.extract(t);
        transition select(h.data, b) {
            (default, true): next;
            (default, default): reject;
        }
    }
    state next {
        p.extract(h);
        transition select(h.data, b) {
            (default, true): accept;
            (default, default): reject;
            default: reject;
        }
    }
}

parser proto(packet_in p, out Header h, out Tst t);
package top(proto _p);
top(p0()) main;

