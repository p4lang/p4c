// Control testcase to ensure that this program actually emits warnings
// that can be disabled with --Wdisable=uninitialized_use and --Wdisable=invalid_header.
header h_t {
    bit<28> f1;
    bit<4> f2;
}

extern void __e(in bit<28> arg);

control C() {
    action foo() {
        h_t h;
        __e(h.f1);
    }

    table t {
        actions = { foo; }
        default_action = foo;
    }

    apply {
        t.apply();
    }
}

control proto();
package top(proto p);

top(C()) main;
