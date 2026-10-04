header H {
    bit<1> z;
}

extern bit<32> f(inout bit<1> x, in bit<1> b);
control c(out H[2] h);
package top(c _c);
control my(out H[2] s) {
    bit<32> hsVar;
    @name("my.a") bit<32> a_0;
    @hidden action array_field23() {
        a_0 = f(s[32w0].z, 1w1);
    }
    @hidden action array_field23_0() {
        a_0 = f(s[32w1].z, 1w1);
    }
    @hidden action array_field23_1() {
        a_0 = hsVar;
    }
    @hidden action array_field19() {
        a_0 = 32w0;
        s[32w0].z = 1w1;
        s[32w1].z = 1w0;
        a_0 = f(s[32w0].z, 1w0);
    }
    @hidden table tbl_array_field19 {
        actions = {
            array_field19();
        }
        const default_action = array_field19();
    }
    @hidden table tbl_array_field23 {
        actions = {
            array_field23();
        }
        const default_action = array_field23();
    }
    @hidden table tbl_array_field23_0 {
        actions = {
            array_field23_0();
        }
        const default_action = array_field23_0();
    }
    @hidden table tbl_array_field23_1 {
        actions = {
            array_field23_1();
        }
        const default_action = array_field23_1();
    }
    apply {
        tbl_array_field19.apply();
        if (a_0 == 32w0) {
            tbl_array_field23.apply();
        } else if (a_0 == 32w1) {
            tbl_array_field23_0.apply();
        } else if (a_0 >= 32w1) {
            tbl_array_field23_1.apply();
        }
    }
}

top(my()) main;
