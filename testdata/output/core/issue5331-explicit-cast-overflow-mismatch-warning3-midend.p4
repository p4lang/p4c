extern void __e(in bit<4> x);
control C() {
    @name("C.a") action a() {
        __e(4w8);
    }
    @name("C.t") table t_0 {
        actions = {
            a();
        }
        default_action = a();
    }
    apply {
        t_0.apply();
    }
}

control proto();
package top(proto p);
top(C()) main;
