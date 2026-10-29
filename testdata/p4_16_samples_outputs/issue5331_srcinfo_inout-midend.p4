extern void __e(in bit<16> x);
control C(in bit<16> x) {
    @name("C.a") action a() {
        __e(x << 6w18);
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

control proto(in bit<16> x);
package top(proto p);
top(C()) main;
