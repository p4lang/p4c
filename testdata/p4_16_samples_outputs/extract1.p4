#include "/home/mbudiu/git/p4c/p4include/core.p4"

header H {
    bit<32> field;
}

parser P(packet_in p, out H[2] h) {
    state start {
        p.extract(h.next);
        p.extract(h.next);
        p.extract(h.last);
    }
}

parser Simple(packet_in p, out H[2] h);
package top(Simple prs);
top(P()) main;
