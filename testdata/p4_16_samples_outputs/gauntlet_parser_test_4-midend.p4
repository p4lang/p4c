#include <core.p4>

header Hdr {
    bit<1> x;
}

struct Headers {
    Hdr[2] h1;
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
        transition select((bit<1>)(h.h1[0].x == 1w1)) {
            1w1: start_true;
            1w0: start_join;
            default: noMatch;
        }
    }
    state start1 {
        transition stateOutOfBound;
    }
    state start_join {
        transition select(32w0) {
            32w0: accept;
            default: start;
        }
    }
    state start_join1 {
        transition select(32w1) {
            32w0: accept;
            default: start1;
        }
    }
    state start_true {
        p.extract<Hdr>(h.h1[32w1]);
        transition start_join1;
    }
}

parser Simple<T>(packet_in p, out T t);
package top<T>(Simple<T> prs);
top<Headers>(P()) main;
