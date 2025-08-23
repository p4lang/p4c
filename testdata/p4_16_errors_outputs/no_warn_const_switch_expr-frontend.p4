control C(out bit<8> y) {
    @name("C.x_0") bit<2> x;
    @name("C.y_0") bit<8> y_1;
    @name("C.foo") action foo() {
        x = 2w1;
        switch (x) {
            2w0b1: {
                y_1 = 8w2;
            }
            2w0b10: {
                y_1 = 8w3;
            }
        }
        y = y_1;
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

control proto(out bit<8> y);
package top(proto p);
top(C()) main;
