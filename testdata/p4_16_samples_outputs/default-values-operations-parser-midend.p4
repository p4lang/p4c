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

parser p(out S s) {
    H argTmp_1_h1;
    H argTmp_1_h2;
    H argTmp_2_h1;
    H argTmp_2_h2;
    H s1_1_h2;
    H s1_3_h2;
    state start {
        argTmp_1_h1.setInvalid();
        argTmp_1_h2.setInvalid();
        argTmp_1_h1.setValid();
        argTmp_1_h1.a = 32w11;
        argTmp_1_h1.b = 32w22;
        s1_1_h2.setValid();
        s1_1_h2.a = 32w1;
        s1_1_h2.b = 32w2;
        s.h1 = argTmp_1_h1;
        argTmp_2_h1.setValid();
        argTmp_2_h2.setValid();
        argTmp_2_h1.a = 32w1;
        argTmp_2_h1.b = 32w0;
        argTmp_2_h2.a = 32w2;
        argTmp_2_h2.b = 32w0;
        s1_3_h2.setValid();
        s1_3_h2.a = 32w1;
        s1_3_h2.b = 32w2;
        s.h2 = s1_3_h2;
        s.c = 32w0;
        transition accept;
    }
}

parser empty(out S s);
package top(empty e);
top(p()) main;

