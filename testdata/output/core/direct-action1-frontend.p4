control c(inout bit<16> y) {
    @name("c.x") bit<32> x_0;
    @name("c.arg") bit<32> arg_0;
    @name("c.a") action a() {
        arg_0 = x_0;
        y = (bit<16>)arg_0;
    }
    apply {
        x_0 = 32w10;
        a();
    }
}

control proto(inout bit<16> y);
package top(proto _p);
top(c()) main;
