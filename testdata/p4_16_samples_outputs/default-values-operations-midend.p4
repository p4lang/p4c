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
    H argTmp_0_h;
    H returnTmp_0_h;
    @hidden action defaultvaluesoperations27() {
        argTmp_0_h.setInvalid();
        returnTmp_0_h.setInvalid();
        r.i = 1w1;
    }
    @hidden table tbl_defaultvaluesoperations27 {
        actions = {
            defaultvaluesoperations27();
        }
        const default_action = defaultvaluesoperations27();
    }
    apply {
        tbl_defaultvaluesoperations27.apply();
    }
}

control simple(inout S1 r);
package top(simple e);
top(c()) main;

