control c(inout bit<32> x) {
    @name("arg") bit<32> arg_0;
    @name("a") action a_0() {
        arg_0 = 32w10;
        x = arg_0;
    }
    @name("t") table t() {
        actions = {
            a_0();
        }
        default_action = a_0();
    }
    apply {
        t.apply();
    }
}

control proto(inout bit<32> arg);
package top(proto p);
top(c()) main;
