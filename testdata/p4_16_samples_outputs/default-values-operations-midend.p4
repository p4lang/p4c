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
    H argTmp_0_h;
    H s_0_h;
    H returnTmp_0_h;
    @hidden action defaultvaluesoperations20() {
        argTmp_0_h.setValid();
        argTmp_0_h.a = 1w1;
        argTmp_0_h.b = true;
        argTmp_0_h.c = 32w2;
        s_0_h.setValid();
        s_0_h.a = 1w0;
        s_0_h.b = false;
        s_0_h.c = 32w1;
        returnTmp_0_h.setInvalid();
        r.b = true;
        r.i = 1w1;
    }
    @hidden table tbl_defaultvaluesoperations20 {
        actions = {
            defaultvaluesoperations20();
        }
        const default_action = defaultvaluesoperations20();
    }
    apply {
        tbl_defaultvaluesoperations20.apply();
    }
}

control simple(inout S r);
package top(simple e);
top(c()) main;

