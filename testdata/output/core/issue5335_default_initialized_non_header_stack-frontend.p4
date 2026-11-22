struct S {
    bit<8> f;
}

control C(out bit<8> x) {
    @name("C.a") action a(@name("stk") S[4] stk) {
        x = stk[1].f;
    }
    @name("C.t") table t_0 {
        actions = {
            a();
        }
        default_action = a((S[4]){(S){f = 8w0},(S){f = 8w0},(S){f = 8w0},(S){f = 8w0}});
    }
    apply {
        t_0.apply();
    }
}

control proto(out bit<8> x);
package top(proto p);
top(C()) main;
