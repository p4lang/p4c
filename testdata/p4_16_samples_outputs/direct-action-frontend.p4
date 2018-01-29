control c(inout bit<16> y) {
    bit<32> x;
    @name("c.a") action a_0(in bit<32> arg) {
        y = (bit<16>)arg;
    }
    apply {
        x = 32w2;
        a_0(x);
    }
}

control proto(inout bit<16> y);
package top(proto _p);
top(c()) main;

