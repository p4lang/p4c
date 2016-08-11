#include "/home/cdodd/p4c/p4include/core.p4"

header H {
    bit<32> field;
}

parser P(packet_in p, out H h) {
    state start {
        p.extract(h, 32);
    }
}

parser Simple(packet_in p);
package top(Simple prs);
top(P()) main;
