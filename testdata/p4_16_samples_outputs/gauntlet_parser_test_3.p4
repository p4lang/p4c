#include <core.p4>

header Hdr {
    bit<8> x;
}

struct Headers {
    Hdr[2] h1;
    Hdr[2] h2;
}

parser P(packet_in p, out Headers h) {
    state start {
        p.extract(h.h1.next);
        p.extract(h.h1.next);
        if (h.h1[1].x == 1) {
            h.h2[1] = h.h1.next;
        } else {
            h.h2[1] = h.h1.last;
        }
        transition accept;
    }
}

parser Simple<T>(packet_in p, out T t);
package top<T>(Simple<T> prs);
top(P()) main;
