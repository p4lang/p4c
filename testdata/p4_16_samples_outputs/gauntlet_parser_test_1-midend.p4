#include <core.p4>

header Hdr {
    bit<8> x;
}

struct Headers {
    Hdr[2] h1;
    Hdr[2] h2;
}

parser P(packet_in p, out Headers h) {
    state stateOutOfBound {
        verify(false, error.StackOutOfBounds);
        transition reject;
    }
    state noMatch {
        verify(false, error.NoMatch);
        transition reject;
    }
    state start {
        p.extract<Hdr>(h.h1[32w0]);
        p.extract<Hdr>(h.h1[32w1]);
        transition select((bit<1>)(h.h1[1].x == 8w1)) {
            1w1: start_true;
            1w0: start_join;
            default: noMatch;
        }
    }
    state start_join {
        h.h2 = h.h1;
        transition accept;
    }
    state start_true {
        transition stateOutOfBound;
    }
}

parser Simple<T>(packet_in p, out T t);
package top<T>(Simple<T> prs);
top<Headers>(P()) main;
