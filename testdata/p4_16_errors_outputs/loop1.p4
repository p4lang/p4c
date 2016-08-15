#include "/home/mbudiu/git/p4c/p4include/core.p4"

header B {
}

parser P(packet_in p) {
    B b;
    state start {
        p.extract(b);
        transition start;
    }
}

parser Simple(packet_in p);
package top(Simple prs);
top(P()) main;
