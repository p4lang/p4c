control c(inout bit<32> x) {
    bit<32> b_0;
    @name("a") action a(bit<32> d) {
        b_0 = x;
        b_0 = d;
        x = b_0;
    }
    @name("t") table t_0() {
        actions = {
            a();
        }
        default_action = a(32w0);
    }
    apply {
        t_0.apply();
    }
}

control proto(inout bit<32> x);
package top(proto p);
top(c()) main;
