extern void __e(in bit<28> arg);
extern void __e2(in bit<28> arg);
control C() {
    @name("C.x") bit<28> x_0;
    @name("C.y") bit<28> y_0;
    @name("C.a") bool a;
    @name("C.b") bool b;
    @name("C.a") bool a_2;
    @name("C.b") bool b_2;
    @name("C.foo") action foo() {
        a = true;
        b = false;
        if (a) {
            if (b) {
                __e(x_0);
            }
        } else if (b) {
            __e2(y_0);
        }
        a_2 = true;
        b_2 = false;
        if (a_2) {
            if (b_2) {
                __e(x_0);
            }
        } else if (b_2) {
            __e2(y_0);
        }
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
