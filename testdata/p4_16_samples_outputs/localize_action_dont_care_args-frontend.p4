struct S {
    bit<64> f;
}

control C(inout S s) {
    @name("C.foo") action foo() {
    }
    @name("C.t") table t_0 {
        actions = {
            foo();
        }
        default_action = foo();
    }
    apply {
        t_0.apply();
    }
}

control proto(inout S s);
package top(proto p);
top(C()) main;
