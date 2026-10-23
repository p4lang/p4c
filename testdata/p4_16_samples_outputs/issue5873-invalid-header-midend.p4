#include <core.p4>

header h1_t {
    bit<8> f1;
}

struct S {
    h1_t h;
}

control C(inout S s, in bit<8> k, out bool v) {
    h1_t ih;
    h1_t ih_0;
    @name("C.a") action a() {
        ih.setInvalid();
        s.h = ih;
    }
    @name("C.a") action a_1() {
        ih_0.setInvalid();
        s.h = ih_0;
    }
    @name("C.t1") table t1_0 {
        actions = {
            a();
        }
        default_action = a();
    }
    @name("C.t2") table t2_0 {
        key = {
            k: exact @name("k");
        }
        actions = {
            a_1();
        }
        default_action = a_1();
    }
    @hidden action issue5873invalidheader31() {
        v = s.h.isValid();
    }
    @hidden table tbl_issue5873invalidheader31 {
        actions = {
            issue5873invalidheader31();
        }
        const default_action = issue5873invalidheader31();
    }
    apply {
        t1_0.apply();
        tbl_issue5873invalidheader31.apply();
        t2_0.apply();
    }
}

control proto(inout S s, in bit<8> k, out bool v);
package top(proto p);
top(C()) main;
