header H {
    bit<1> z;
}

extern bit<32> f(inout bit<1> x, in bit<1> b);
control c(out H[2] h);
package top(c _c);
control my(out H[2] s) {
    bit<32> tmp;
    @hidden action array_field27() {
        s[32w0].z = 1w1;
        s[32w1].z = 1w0;
        tmp = f(s[32w0].z, 1w0);
        f(s[tmp].z, 1w1);
    }
    @hidden table tbl_array_field27 {
        actions = {
            array_field27();
        }
        const default_action = array_field27();
    }
    apply {
        tbl_array_field27.apply();
    }
}

top(my()) main;

