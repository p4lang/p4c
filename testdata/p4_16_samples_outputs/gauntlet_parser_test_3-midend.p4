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
        transition select((bit<1>)(h.h1[1].x == 8w1)) {
            1w1: start_true;
            1w0: start_false;
            default: noMatch;
        }
    }
    state start_true {
        h.h2[1] = h.h1.next;
        transition start_join;
    }
    state start_false {
        h.h2[1] = h.h1.last;
        transition start_join;
    }
    state start_join {
        transition accept;
    }
    state noMatch {
        verify(false, error.NoMatch);
        transition reject;
    }
}

parser Simple<T>(packet_in p, out T t);
package top<T>(Simple<T> prs);
top<Headers>(P()) main;
