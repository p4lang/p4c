struct S1 {
    bit<8> f1;
}

struct S2 {
    S1     s1;
    bit<9> f2;
}

control C(out bit<8> x) {
    @name("C.a") action a(@name("stk") S2[4][2] stk) {
        x = stk[1][0].s1.f1;
    }
    @name("C.t") table t_0 {
        actions = {
            a();
        }
        default_action = a((S2[4][2]){(S2[4]){(S2){s1 = (S1){f1 = 8w0},f2 = 9w0},(S2){s1 = (S1){f1 = 8w0},f2 = 9w0},(S2){s1 = (S1){f1 = 8w0},f2 = 9w0},(S2){s1 = (S1){f1 = 8w0},f2 = 9w0}},(S2[4]){(S2){s1 = (S1){f1 = 8w0},f2 = 9w0},(S2){s1 = (S1){f1 = 8w0},f2 = 9w0},(S2){s1 = (S1){f1 = 8w0},f2 = 9w0},(S2){s1 = (S1){f1 = 8w0},f2 = 9w0}}});
    }
    apply {
        t_0.apply();
    }
}

control proto(out bit<8> x);
package top(proto p);
top(C()) main;
