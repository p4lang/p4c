extern bit<32> f(in bit<32> x);
control c(inout bit<32> r) {
    apply {
        if (f(2) > 0 && f(3) < 0) {
            r = 1;
        } else {
            r = 2;
        }
    }
}

control simple(inout bit<32> r);
package top(simple e);
top(c()) main;

