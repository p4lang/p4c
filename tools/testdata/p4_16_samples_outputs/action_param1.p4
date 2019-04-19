control c(inout bit<32> x) {
    action a(in bit<32> arg) {
        x = arg;
    }
    apply {
        a(15);
    }
}

control proto(inout bit<32> arg);
package top(proto p);
top(c()) main;

