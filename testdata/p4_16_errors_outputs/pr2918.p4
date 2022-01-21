#include <core.p4>

struct S {
    bit<8> b;
}

header H {
    bit<8> b;
    S      s;
}

struct Headers {
    H h;
}

parser p(packet_in p, out Headers h) {
    state start {
        p.extract(h.h);
        transition select(h.h.s) {
            {b = 0}: reject;
            {b = 1}: accept;
        }
    }
}

