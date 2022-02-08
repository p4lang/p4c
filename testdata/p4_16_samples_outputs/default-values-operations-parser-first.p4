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
    S s1 = (S){h1 = s.h1,h2 = (H){a = 32w1,b = 32w2},c = 32w0};
    return s1;
}
parser p(out S s) {
    state start {
        S argTmp;
        argTmp.h1 = (H){a = 32w11,b = 32w22};
        argTmp.c = 32w0;
        S s1 = f(argTmp);
        s.h1 = s1.h1;
        transition next;
    }
    state next {
        S argTmp_0 = (S){h1 = (H){a = 32w1,b = 32w0},h2 = (H){a = 32w2,b = 32w0},c = 32w0};
        S s2 = f(argTmp_0);
        s.h2 = s2.h2;
        s.c = 32w0;
        transition accept;
    }
}

parser empty(out S s);
package top(empty e);
top(p()) main;

