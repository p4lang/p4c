control C(out bit<64> v64) {
    @name("C.a") action a() {
        v64 = 64w18446744073709551615;
    }
    @name("C.t") table t_0 {
        actions = {
            a();
        }
        default_action = a();
    }
    apply {
        t_0.apply();
    }
}

control proto(out bit<64> v64);
package top(proto p);
top(C()) main;
