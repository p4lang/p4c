extern void __e(in bit<4> x);
control C() {
    action a() {
        __e(4w8);
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

control proto();
package top(proto p);
top(C()) main;
