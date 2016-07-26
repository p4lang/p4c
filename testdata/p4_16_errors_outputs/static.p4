#include "/home/cdodd/p4c/p4include/core.p4"

parser P(packet_in p) {
    bit<32> x;
    state start {
        x = 2;
        transition next;
    }
    state next {
        x = x / (x - 2);
    }
}

parser Simple(packet_in p);
package top(Simple prs);
top(P()) main;
