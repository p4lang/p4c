control c(inout bit<32> x) {
    @name("a") action a_0(bit<32> d) {
        x = d;
    }
    @name("t") table t {
        actions = {
            a_0();
        }
        default_action = a_0(32w0);
    }
    apply {
        t.apply();
    }
}

control proto(inout bit<32> x);
package top(proto p);
top(c()) main;

