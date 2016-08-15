#include "/home/cdodd/p4c/p4include/core.p4"

header H {
    bit<32> field;
}

parser P(packet_in p, out H h) {
    state start {
        p.extract(h);
        p.extract(h);
    }
}

parser Simple(packet_in p, out H h);
package top(Simple prs);
top(P()) main;
