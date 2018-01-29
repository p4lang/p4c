control c(inout bit<32> x) {
    @name("c.a") action a_0(in bit<32> arg_1) {
        x = arg_1;
    }
    @name("c.t") table t {
        actions = {
            a_0(32w10);
        }
        default_action = a_0(32w10);
    }
    apply {
        t.apply();
    }
}

control proto(inout bit<32> arg);
package top(proto p);
top(c()) main;

