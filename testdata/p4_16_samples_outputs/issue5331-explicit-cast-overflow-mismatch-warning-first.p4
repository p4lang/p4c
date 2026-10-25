control C(out bit<64> v64) {
    action a() {
        v64 = 64w18446744073709551615;
    }
    table t {
        actions = {
            a();
        }
        default_action = a();
    }
    apply {
        t.apply();
    }
}

control proto(out bit<64> v64);
package top(proto p);
top(C()) main;
