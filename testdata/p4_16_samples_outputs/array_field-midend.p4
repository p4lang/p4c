header H {
    bit<1> z;
}

extern bit<32> f(inout bit<1> x, in bit<1> b);
control c(out H[2] h);
package top(c _c);
control my(out H[2] s) {
    @name("my.a") bit<32> a_0;
    @hidden action array_field26() {
        a_0 = 32w0;
        s[32w0].z = 1w1;
        s[32w1].z = 1w0;
        a_0 = f(s[32w0].z, 1w0);
        a_0 = f(s[a_0].z, 1w1);
    }
    @hidden table tbl_array_field26 {
        actions = {
            array_field26();
        }
        const default_action = array_field26();
    }
    apply {
        tbl_array_field26.apply();
    }
}

top(my()) main;

