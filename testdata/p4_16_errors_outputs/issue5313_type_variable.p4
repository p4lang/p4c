@command_line("--preferSwitch") void bar_generic<T>(T v, out bit<8> y) {
    switch (v) {
        0b1: {
            y = 1;
        }
        0b10: {
            y = 2;
        }
        default: {
            y = 0;
        }
    }
}
control C(out bit<8> y) {
    action foo() {
        bar_generic<bit<2>>(1, y);
    }
    table t {
        actions = {
            foo;
        }
        default_action = foo;
    }
    apply {
        t.apply();
    }
}

control proto(out bit<8> y);
package top(proto p);
top(C()) main;
