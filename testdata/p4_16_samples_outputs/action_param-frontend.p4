control c(inout bit<32> x) {
    @name("a") action a_0(in bit<32> arg) {
        x = arg;
    }
    @name("t") table t_0() {
        actions = {
            a_0(32w10);
        }
        default_action = a_0();
    }
    apply {
        t_0.apply();
    }
}

control proto(inout bit<32> arg);
package top(proto p);
top(c()) main;
