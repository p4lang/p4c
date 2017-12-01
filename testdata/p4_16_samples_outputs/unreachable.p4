#include <core.p4>

header Header {
    bit<32> data;
}

parser p1(packet_in p, out Header h) {
    state start {
        transition next;
    }
    state next {
        p.extract(h);
        transition accept;
    }
    state unreachable1 {
        transition unreachable2;
    }
    state unreachable2 {
        transition unreachable1;
    }
}

parser proto(packet_in p, out Header h);
package top(proto _p);
top(p1()) main;

