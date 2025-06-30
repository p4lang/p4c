struct S1 {
    bit<8> f1;
}
struct S2 {
    S1 s1;
    bit<9> f2;
}
control C(out bit<8> x) {
    action a(S2[4][2] stk) {
        x = stk[1][0].s1.f1;
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
