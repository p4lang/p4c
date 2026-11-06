#include <core.p4>

header h1_t {
    bit<8> f1;
}

header h2_t {
    bit<8> f2;
}

struct S {
    h1_t hu_h1;
    h2_t hu_h2;
}

control C(inout S s, in bit<8> k, out bool v) {
    h1_t ih_h1;
    h2_t ih_h2;
    h1_t ih_0_h1;
    h2_t ih_0_h2;
    @name("C.a") action a() {
        if (ih_h1.isValid()) {
            s.hu_h1.setValid();
            s.hu_h1 = ih_h1;
            s.hu_h2.setInvalid();
        } else {
            s.hu_h1.setInvalid();
        }
        if (ih_h2.isValid()) {
            s.hu_h2.setValid();
            s.hu_h2 = ih_h2;
            s.hu_h1.setInvalid();
        } else {
            s.hu_h2.setInvalid();
        }
    }
    @name("C.a") action a_1() {
        if (ih_0_h1.isValid()) {
            s.hu_h1.setValid();
            s.hu_h1 = ih_0_h1;
            s.hu_h2.setInvalid();
        } else {
            s.hu_h1.setInvalid();
        }
        if (ih_0_h2.isValid()) {
            s.hu_h2.setValid();
            s.hu_h2 = ih_0_h2;
            s.hu_h1.setInvalid();
        } else {
            s.hu_h2.setInvalid();
        }
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
    @hidden action issue5873invalidheaderunion35() {
        bit<32> tmp;
        tmp = 32w0;
        if (s.hu_h1.isValid()) {
            tmp = tmp + 32w1;
        }
        if (s.hu_h2.isValid()) {
            tmp = tmp + 32w1;
        }
        v = tmp == 32w1;
    }
    @hidden table tbl_issue5873invalidheaderunion35 {
        actions = {
            issue5873invalidheaderunion35();
        }
        const default_action = issue5873invalidheaderunion35();
    }
    apply {
        t1_0.apply();
        tbl_issue5873invalidheaderunion35.apply();
        t2_0.apply();
    }
}

control proto(inout S s, in bit<8> k, out bool v);
package top(proto p);
top(C()) main;
