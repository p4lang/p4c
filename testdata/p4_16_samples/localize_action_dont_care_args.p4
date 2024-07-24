struct S {
    bit<64> f;
}

control C(inout S s) {
    action d(out bit<64> b) {
        b = 4;
    }

    action foo() {
        d(_);
    }

    table t {
        actions = { foo; }
        default_action = foo;
    }

    apply {
        t.apply();
    }
}

control proto(inout S s);
package top(proto p);

top(C()) main;
