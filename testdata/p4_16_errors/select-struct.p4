#include <core.p4>

struct H {
    bit<12> b;
}

parser prs(packet_in p) {
    state start {
        H h1 = { 0 };
        H h2 = { 1 };
        transition select (h1) {
            h2: accept;
            _ : reject;
        }
    }
}

parser P(packet_in p);
package top(P _p);

top(prs()) main;
