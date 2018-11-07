control c(inout bit<32> x) {
    @name("c.a") action a(in bit<32> arg_1) {
        x = arg_1;
    }
    @name("c.t") table t_0 {
        actions = {
            a(32w10);
        }
        default_action = a(32w10);
    }
    apply {
        t_0.apply();
    }
}

control proto(inout bit<32> arg);
package top(proto p);
top(c()) main;

