#include <core.p4>

header H {
    bit<32> a;
    bit<32> b;
}

struct S {
    H       h1;
    H       h2;
    bit<32> c;
}

parser p() {
    state start {
        S s;
        s.c = 32w0;
        transition accept;
    }
}

parser empty();
package top(empty e);
top(p()) main;

