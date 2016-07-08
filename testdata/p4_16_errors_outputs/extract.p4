#include "/home/mbudiu/git/p4c/p4include/core.p4"

parser P(packet_in p, out bit<32> h) {
    state start {
        p.extract(h);
    }
}

parser Simple(packet_in p);
package top(Simple prs);
top(P()) main;
