extern void __e(in bit<16> x);
control C(in bit<16> x) {
    @name("C.value") bit<16> value_0;
    @name("C.y") bit<16> y;
    @name("C.a") action a() {
        value_0 = x << 6w18;
        y = value_0;
        __e(y);
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
