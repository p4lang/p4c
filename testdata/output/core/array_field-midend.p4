header H {
    bit<1> z;
}

extern bit<32> f(inout bit<1> x, in bit<1> b);
control c(out H[2] h);
package top(c _c);
control my(out H[2] s) {
    bit<32> hsVar;
    @name("my.a") bit<32> a_0;
    @hidden action array_field21() {
        a_0 = f(s[32w0].z, 1w1);
    }
    @hidden action array_field21_0() {
        a_0 = f(s[32w1].z, 1w1);
    }
    @hidden action array_field21_1() {
        a_0 = hsVar;
    }
    @hidden action array_field17() {
        a_0 = 32w0;
        s[32w0].z = 1w1;
        s[32w1].z = 1w0;
        a_0 = f(s[32w0].z, 1w0);
    }
    @hidden table tbl_array_field17 {
        actions = {
            array_field17();
        }
        const default_action = array_field17();
    }
    @hidden table tbl_array_field21 {
        actions = {
            array_field21();
        }
        const default_action = array_field21();
    }
    @hidden table tbl_array_field21_0 {
        actions = {
            array_field21_0();
        }
        const default_action = array_field21_0();
    }
    @hidden table tbl_array_field21_1 {
        actions = {
            array_field21_1();
        }
        const default_action = array_field21_1();
    }
    apply {
        tbl_array_field17.apply();
        if (a_0 == 32w0) {
            tbl_array_field21.apply();
        } else if (a_0 == 32w1) {
            tbl_array_field21_0.apply();
        } else if (a_0 >= 32w1) {
            tbl_array_field21_1.apply();
        }
    }
}

top(my()) main;
