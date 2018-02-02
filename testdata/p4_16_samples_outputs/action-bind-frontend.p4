control c(inout bit<32> x) {
    @name("c.a") action a_0(inout bit<32> b, bit<32> d) {
        b = d;
    }
    @name("c.t") table t {
        actions = {
            a_0(x);
        }
        default_action = a_0(x, 32w0);
    }
    apply {
        t.apply();
    }
}

control proto(inout bit<32> x);
package top(proto p);
top(c()) main;

