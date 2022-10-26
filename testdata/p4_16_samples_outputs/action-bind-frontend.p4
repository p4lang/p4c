control c(inout bit<32> x) {
    @name("c.b") bit<32> b_0;
    @name("c.a") action a(@name("d") bit<32> d) {
        b_0 = d;
        x = b_0;
    }
    @name("c.t") table t_0 {
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
