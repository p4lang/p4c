@command_line("--preferSwitch")

// Generic helper over a type variable T.
// The switch is written in terms of T; T is instantiated later.
void bar_generic<T>(T v, out bit<8> y) {
    switch (v) {
        0b01: { y = 1; }
        0b10: { y = 2; }
        default: { y = 0; }
    }
}

control C(out bit<8> y) {
    action foo() {
        // Here T is instantiated as bit<2>, so switch(v) is on bit<2>.
        bar_generic<bit<2>>(1, y);
    }

    table t {
        actions = { foo; }
        default_action = foo;
    }

    apply {
        t.apply();
    }
}

control proto(out bit<8> y);
package top(proto p);

top(C()) main;
