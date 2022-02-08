#include <core.p4>

header H {
    bit<1>  a;
    bool    b;
    bit<32> c;
}

struct S1 {
    bit<1> i;
    H      h;
    bool   b;
}

struct S {
    bit<32> a;
    bit<32> b;
}

control c(inout S1 r) {
    @name("c.argTmp") S1 argTmp_0;
    @name("c.val") S1 val_0;
    @name("c.data_0") S1 data;
    @name("c.hasReturned") bool hasReturned;
    @name("c.retval") S1 retval;
    @name("c.i") bit<1> i_0;
    @name("c.returnTmp") S1 returnTmp_0;
    apply {
        argTmp_0.h.setInvalid();
        argTmp_0.i = 1w1;
        argTmp_0.b = false;
        data = argTmp_0;
        hasReturned = false;
        i_0 = data.i;
        returnTmp_0.h.setInvalid();
        returnTmp_0.i = i_0;
        returnTmp_0.b = false;
        hasReturned = true;
        retval = returnTmp_0;
        val_0 = retval;
        r.i = val_0.i;
    }
}

control simple(inout S1 r);
package top(simple e);
top(c()) main;

