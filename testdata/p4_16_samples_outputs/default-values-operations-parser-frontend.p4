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
    @name("p.argTmp") S argTmp_1;
    @name("p.s1") S s1_0;
    @name("p.argTmp_0") S argTmp_2;
    @name("p.s2") S s2_0;
    @name("p.s_0") S s_2;
    @name("p.hasReturned") bool hasReturned;
    @name("p.retval") S retval;
    @name("p.s1") S s1_1;
    @name("p.s_1") S s_3;
    @name("p.hasReturned") bool hasReturned_1;
    @name("p.retval") S retval_1;
    @name("p.s1") S s1_3;
    state start {
        argTmp_1.h1.setInvalid();
        argTmp_1.h2.setInvalid();
        argTmp_1.h1.setValid();
        argTmp_1.h1 = (H){a = 32w11,b = 32w22};
        argTmp_1.c = 32w0;
        s_2 = argTmp_1;
        hasReturned = false;
        s1_1.h2.setValid();
        s1_1 = (S){h1 = s_2.h1,h2 = (H){a = 32w1,b = 32w2},c = 32w0};
        hasReturned = true;
        retval = s1_1;
        s1_0 = retval;
        s.h1 = s1_0.h1;
        argTmp_2.h1.setValid();
        argTmp_2.h2.setValid();
        argTmp_2 = (S){h1 = (H){a = 32w1,b = 32w0},h2 = (H){a = 32w2,b = 32w0},c = 32w0};
        s_3 = argTmp_2;
        hasReturned_1 = false;
        s1_3.h2.setValid();
        s1_3 = (S){h1 = s_3.h1,h2 = (H){a = 32w1,b = 32w2},c = 32w0};
        hasReturned_1 = true;
        retval_1 = s1_3;
        s2_0 = retval_1;
        s.h2 = s2_0.h2;
        s.c = 32w0;
        transition accept;
    }
}

parser empty(out S s);
package top(empty e);
top(p()) main;

