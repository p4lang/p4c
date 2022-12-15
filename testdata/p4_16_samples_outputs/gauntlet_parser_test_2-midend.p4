#include <core.p4>

header Hdr {
    bit<8> x;
}

struct Headers {
    Hdr[2] h1;
}

parser P(packet_in p, out Headers h) {
    state stateOutOfBound {
        verify(false, error.StackOutOfBounds);
        transition reject;
    }
    state start {
        p.extract<Hdr>(h.h1[32w0]);
        transition select(h.h1[32w0].x) {
            8w0: start1;
            default: accept;
        }
    }
    state start1 {
        p.extract<Hdr>(h.h1[32w1]);
        transition select(h.h1[32w1].x) {
            8w0: start2;
            default: accept;
        }
    }
    state start2 {
        transition stateOutOfBound;
    }
}

parser Simple<T>(packet_in p, out T t);
package top<T>(Simple<T> prs);
top<Headers>(P()) main;
