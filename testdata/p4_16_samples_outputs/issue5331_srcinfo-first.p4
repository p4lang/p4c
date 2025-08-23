extern void __e(in bit<16> x);
control C(in bit<16> x) {
    action a3(bit<16> y) {
        __e(y);
    }
    action a2(bit<6> y) {
        a3(x << y + 6w9);
    }
    action a() {
        a2(6w9);
    }
    table t {
        actions = {
            a();
        }
        default_action = a();
    }
    apply {
        t.apply();
    }
}

control proto(in bit<16> x);
package top(proto p);
top(C()) main;
