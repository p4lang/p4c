control c(inout bit<32> x) {
    action a(in bit<32> arg) {
        x = arg;
    }
    table t {
        actions = {
            a(32w10);
        }
        default_action = a(32w10);
    }
    apply {
        t.apply();
    }
}

control proto(inout bit<32> arg);
package top(proto p);
top(c()) main;

