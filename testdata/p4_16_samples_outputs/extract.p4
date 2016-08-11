#include "/home/cdodd/p4c/p4include/core.p4"

header H {
    bit<32> field;
}

parser P(packet_in p, out H[2] h) {
    state start {
        p.extract(h.next);
        p.extract(h.next);
        p.extract(h.next);
    }
}

parser Simple<T>(packet_in p, out T t);
package top<T>(Simple<T> prs);
top(P()) main;
