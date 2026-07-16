@command_line("--preferSwitch") typedef bit<2> TwoBits;
void bar(TwoBits x, out bit<8> y) {
    switch (x) {
        0b1: {
            y = 2;
        }
        0b10: {
            y = 3;
        }
    }
}
control C(out bit<8> y) {
    action foo() {
        bar(1, y);
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
