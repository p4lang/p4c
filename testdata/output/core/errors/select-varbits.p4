#include <core.p4>

header H {
    varbit<12> v;
}

struct Headers {
    H h1;
    H h2;
}

parser prs(packet_in p, out Headers h) {
    state start {
        p.extract(h.h1, 12);
        p.extract(h.h2, 12);
        transition select(h.h1.v) {
            h.h2.v: accept;
            default: reject;
        }
    }
}

parser P(packet_in p, out Headers h);
package top(P _p);
top(prs()) main;
