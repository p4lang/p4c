control c(inout bit<32> x) {
    @name("c.a") action a() {
        x = 32w10;
    }
    @name("c.t") table t_0 {
        actions = {
            a();
        }
        default_action = a();
    }
    apply {
        t_0.apply();
    }
}

control proto(inout bit<32> arg);
package top(proto p);
top(c()) main;

