void bar(in bit<2> x, out bit<8> y) {
    switch (x) {
        2w0b1: {
            y = 8w2;
        }
        2w0b10: {
            y = 8w3;
        }
    }
}
control C(out bit<8> y) {
    action foo() {
        bar(2w1, y);
    }
    table t {
        actions = {
            foo();
        }
        default_action = foo();
    }
    apply {
        t.apply();
    }
}

control proto(out bit<8> y);
package top(proto p);
top(C()) main;
