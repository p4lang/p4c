control c(inout bit<16> y) {
    @name("x") bit<32> x_0;
    @name("a") action a_0(bit<32> arg) {
        y = (bit<16>)arg;
    }
    apply {
        x_0 = 32w10;
        a_0(x_0);
    }
}

control proto(inout bit<16> y);
package top(proto _p);
top(c()) main;
