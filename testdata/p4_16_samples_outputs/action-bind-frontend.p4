control c(inout bit<32> x) {
    @name("c.a") action a(inout bit<32> b, bit<32> d) {
        b = d;
    }
    @name("c.t") table t_0 {
        actions = {
            a(x);
        }
        default_action = a(x, 32w0);
    }
    apply {
        t_0.apply();
    }
}

control proto(inout bit<32> x);
package top(proto p);
top(c()) main;

