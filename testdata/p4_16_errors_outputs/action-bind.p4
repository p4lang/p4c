control c(inout bit<32> x) {
    bit<32> y;
    action a(inout bit<32> b, bit<32> d) {
        b = d;
    }
    table t {
        actions = {
            a(x);
        }
        default_action = a(y, 0);
    }
    apply {
        t.apply();
    }
}

control proto(inout bit<32> x);
package top(proto p);
top(c()) main;

