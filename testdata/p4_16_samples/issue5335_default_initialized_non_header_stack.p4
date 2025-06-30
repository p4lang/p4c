struct S {
    bit<8> f;
}
control C(out bit<8> x) {
    action a(S[4] stk) {
        x = stk[1].f;
    }

    table t {
        actions = { a; }
        default_action = a(...);
    }

    apply {
        t.apply();
    }
}

control proto(out bit<8> x);
package top(proto p);

top(C()) main;
