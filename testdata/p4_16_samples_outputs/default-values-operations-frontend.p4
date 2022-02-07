#include <core.p4>

header H {
    bit<1>  a;
    bool    b;
    bit<32> c;
}

struct S {
    bit<1> i;
    H      h;
    bool   b;
}

control c(inout S r) {
    @name("c.argTmp") S argTmp_0;
    @name("c.s") S s_0;
    @name("c.val") S val_0;
    @name("c.hasReturned") bool hasReturned;
    @name("c.retval") S retval;
    @name("c.returnTmp") S returnTmp_0;
    apply {
        argTmp_0.h.setValid();
        argTmp_0 = (S){i = 1w0,h = (H){a = 1w1,b = true,c = 32w2},b = false};
        s_0.h.setValid();
        s_0 = (S){i = 1w0,h = (H){a = 1w0,b = false,c = 32w1},b = true};
        hasReturned = false;
        returnTmp_0.h.setInvalid();
        returnTmp_0.i = 1w1;
        returnTmp_0.b = false;
        hasReturned = true;
        retval = returnTmp_0;
        val_0 = retval;
        r.b = s_0.b;
        r.i = val_0.i;
    }
}

control simple(inout S r);
package top(simple e);
top(c()) main;

