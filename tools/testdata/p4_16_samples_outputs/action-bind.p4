control c(inout bit<32> x) {
    action a(inout bit<32> b, bit<32> d) {
        b = d;
    }
    table t {
        actions = {
            a(x);
        }
        default_action = a(x, 0);
    }
    apply {
        t.apply();
    }
}

control proto(inout bit<32> x);
package top(proto p);
top(c()) main;

