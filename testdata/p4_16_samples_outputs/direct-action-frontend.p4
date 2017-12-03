control c(inout bit<16> y) {
    bit<32> x_0;
    @name("a") action a_0(in bit<32> arg_0) {
        y = (bit<16>)arg_0;
    }
    apply {
        x_0 = 32w2;
        a_0(x_0);
    }
}

control proto(inout bit<16> y);
package top(proto _p);
top(c()) main;

