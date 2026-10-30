#include <core.p4>

header h1_t {
    bit<8> f1;
}

header h2_t {
    bit<8> f2;
}

header_union hu_t {
    h1_t h1;
    h2_t h2;
}

struct S {
    hu_t hu;
}

control C(inout S s, in bit<8> k, out bool v) {
    @name("C.s_local") S s_local_0;
    @name("C.a") action a() {
        s_local_0 = (S){hu = (hu_t){#}};
        s = s_local_0;
    }
    @name("C.a") action a_1() {
        s_local_0 = (S){hu = (hu_t){#}};
        s = s_local_0;
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
    apply {
        t1_0.apply();
        v = s.hu.isValid();
        t2_0.apply();
    }
}

control proto(inout S s, in bit<8> k, out bool v);
package top(proto p);
top(C()) main;
