extern void __e(in bit<28> arg);
extern void __e2(in bit<28> arg);
control C() {
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

control proto();
package top(proto p);
top(C()) main;
