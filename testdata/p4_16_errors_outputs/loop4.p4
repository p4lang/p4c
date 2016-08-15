#include "/home/mbudiu/git/p4c/p4include/core.p4"

header B {
    varbit<120> field1;
}

parser P(packet_in p) {
    B b;
    state start {
        b.setInvalid();
        p.extract(b, 10);
        transition start;
    }
}

parser Simple(packet_in p);
package top(Simple prs);
top(P()) main;
