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
        p.extract<Hdr>(h.h1.next);
        p.extract<Hdr>(h.h1.next);
        transition select(h.h1[1].x == 8w1) {
            true: start_true;
            false: start_join;
        }
    }
    state start_true {
        p.extract<Hdr>(h.h1.next);
        transition start_join;
    }
    state start_join {
        h.h2 = h.h1;
        transition accept;
    }
}

parser Simple<T>(packet_in p, out T t);
package top<T>(Simple<T> prs);
top<Headers>(P()) main;
