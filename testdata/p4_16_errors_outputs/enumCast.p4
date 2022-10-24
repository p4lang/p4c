#include <core.p4>

enum bit<32> X {
    Zero = 0,
    One = 1
}

enum bit<8> E1 {
    e1 = 0,
    e2 = 1,
    e3 = 2
}

enum bit<8> E2 {
    e1 = 10,
    e2 = 11,
    e3 = 12
}

header B {
    X x;
}

struct O {
    B b;
}

parser p(packet_in packet, out O o) {
    state start {
        X y = 1;
        y = 32w1;
        bool bb;
        E1 a = E1.e1;
        E2 b = E2.e2;
        a = b;
        a = E1.e1 + 1;
        a = E1.e1 + E1.e2;
        transition accept;
    }
}

parser proto<T>(packet_in p, out T t);
package top<T>(proto<T> _p);
top(p()) main;
