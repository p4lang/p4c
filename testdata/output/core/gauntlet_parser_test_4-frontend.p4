#include <core.p4>

header Hdr {
    bit<1> x;
}

struct Headers {
    Hdr[2] h1;
}

parser P(packet_in p, out Headers h) {
    state start {
        p.extract<Hdr>(h.h1.next);
        transition select(h.h1[0].x == 1w1) {
            true: start_true;
            false: start_join;
        }
    }
    state start_true {
        p.extract<Hdr>(h.h1.next);
        transition start_join;
    }
    state start_join {
        transition select(h.h1.lastIndex) {
            32w0: accept;
            default: start;
        }
    }
}

parser Simple<T>(packet_in p, out T t);
package top<T>(Simple<T> prs);
top<Headers>(P()) main;
