control c(inout bit<16> y) {
    bit<32> x = 2;
    action a(in bit<32> arg) {
        y = (bit<16>)arg;
    }
    apply {
        a(x);
    }
}

control proto(inout bit<16> y);
package top(proto _p);
top(c()) main;

