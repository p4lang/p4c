@command_line("--preferSwitch") type bit<2> TwoBitsAlias;
void bar(bit<2> x, out bit<8> y) {
    TwoBitsAlias a = (TwoBitsAlias)x;
    switch (a) {
        0b1: {
            y = 4;
        }
        0b10: {
            y = 5;
        }
        default: {
            y = y;
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
