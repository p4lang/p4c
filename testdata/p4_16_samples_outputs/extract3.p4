#include "/home/mbudiu/git/p4c/p4include/core.p4"

header H {
    bit<32> field;
}

parser P(packet_in p, out H h) {
    state start {
        p.extract(h);
        transition next;
    }
    state next {
        p.extract(h);
    }
}

parser Simple<T>(packet_in p, out T t);
package top<T>(Simple<T> prs);
top(P()) main;
