header H {
    bit<1> z;
}

extern bit<1> f(inout bit<1> x, in bit<1> b);
control c(out H[2] h);
package top(c _c);
control my(out H[2] s) {
    bit<32> a;
    bit<32> tmp_7;
    bit<32> tmp_19;
    bit<32> tmp_20;
    bit<32> tmp_21;
    bit<1> tmp_22;
    bit<1> tmp_23;
    bit<32> tmp_24;
    bit<1> tmp_25;
    bit<32> tmp_26;
    bit<32> tmp_27;
    bit<32> tmp_28;
    bit<32> tmp_29;
    bit<1> tmp_30;
    bit<1> tmp_31;
    bit<32> tmp_32;
    bit<32> tmp_33;
    bit<1> tmp_34;
    bit<1> tmp_35;
    bit<32> tmp_36;
    @name("act") action act_0() {
        a = 32w0;
        tmp_7 = 32w0;
        tmp_26 = 32w0;
        s[32w0].z = 1w1;
        tmp_27 = 32w1;
        tmp_19 = 32w1;
        tmp_20 = 32w1;
        tmp_28 = 32w1;
        s[32w1].z = 1w0;
        tmp_21 = 32w0;
        tmp_29 = 32w0;
        tmp_22 = s[32w0].z;
        tmp_30 = s[32w0].z;
        tmp_31 = f(tmp_30, 1w0);
        tmp_22 = tmp_30;
        tmp_23 = tmp_31;
        tmp_32 = tmp_21;
        s[tmp_21].z = tmp_30;
        a = (bit<32>)tmp_31;
        tmp_24 = (bit<32>)tmp_31;
        tmp_33 = (bit<32>)tmp_31;
        tmp_25 = s[(bit<32>)tmp_31].z;
        tmp_34 = s[(bit<32>)tmp_31].z;
        tmp_35 = f(tmp_34, 1w1);
        tmp_25 = tmp_34;
        tmp_36 = tmp_24;
        s[tmp_24].z = tmp_34;
    }
    @name("tbl_act") table tbl_act() {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    apply {
        tbl_act.apply();
    }
}

top(my()) main;
