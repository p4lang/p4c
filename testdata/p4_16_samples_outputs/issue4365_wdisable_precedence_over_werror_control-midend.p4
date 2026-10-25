header h_t {
    bit<28> f1;
    bit<4>  f2;
}

extern void __e(in bit<28> arg);
control C() {
    @name("C.h") h_t h_0;
    @name("C.foo") action foo() {
        h_0.setInvalid();
        __e(h_0.f1);
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

control proto();
package top(proto p);
top(C()) main;
