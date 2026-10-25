control C(out bit<64> v64) {
    @name("C.value") bit<1> value;
    @name("C.a") action a() {
        value = 1w1;
        v64 = (bit<64>)(int<64>)(int<1>)value;
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
