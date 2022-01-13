control C(out bit<16> result) {
    @name("C.x") bit<32> x_0;
    @name("C.y") bit<16> y_0;
    apply {
        x_0 = 32w5;
        y_0 = 16w6;
        result = (bit<16>)x_0 + y_0;
    }
}

control CT(out bit<16> r);
package top(CT _c);
top(C()) main;

