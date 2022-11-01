control c(inout bit<32> x) {
    @name("c.arg") bit<32> arg_0;
    @name("c.a") action a() {
        arg_0 = 32w10;
        x = arg_0;
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
