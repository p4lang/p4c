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

S f(in S s) {
    S s1 = {h2 = {a = 1,b = 2},h1 = s.h1, ... };
    return s1;
}
parser p(out S s) {
    state start {
        S s1 = f({h1 = { 11, 22 }, ... });
        s.h1 = s1.h1;
        transition next;
    }
    state next {
        S s2 = f({ { 1, ... }, { 2, ... }, ... });
        s.h2 = s2.h2;
        s.c = 0;
        transition accept;
    }
}

parser empty(out S s);
package top(empty e);
top(p()) main;

