#include <core.p4>

header Hdr {
    bit<1> x;
}

struct Headers {
    Hdr[2] h1;
}


parser P(packet_in p, out Headers h) {
    state start {
        p.extract(h.h1.next);
        if (h.h1[0].x == 1) {
            p.extract(h.h1.next);
        }
        transition select(h.h1.lastIndex) {
            0: accept;
            _: start;
        }
    }
}

parser Simple<T>(packet_in p, out T t);
package top<T>(Simple<T> prs);
top(P()) main;
