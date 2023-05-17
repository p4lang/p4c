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
        transition select((bit<1>)(h.h1[0].x == 1w1)) {
            1w1: start_true;
            1w0: start_join;
            default: noMatch;
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
    state noMatch {
        verify(false, error.NoMatch);
        transition reject;
    }
}

parser Simple<T>(packet_in p, out T t);
package top<T>(Simple<T> prs);
top<Headers>(P()) main;
