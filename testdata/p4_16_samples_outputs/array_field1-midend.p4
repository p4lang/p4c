header H {
    bit<1> z;
}

extern bit<1> f(inout bit<1> x, in bit<1> b);
control c(out H[2] h);
package top(c _c);
control my(out H[2] s) {
<<<<<<< 03c3c3eb92dc48804072b130de141a2114808c3b
    bit<32> tmp_21;
    bit<32> tmp_24;
    bit<1> tmp_30;
    bit<1> tmp_31;
    bit<1> tmp_34;
    @name("act") action act_0() {
        s[32w0].z = 1w1;
        s[32w1].z = 1w0;
        tmp_21 = 32w0;
        tmp_30 = s[32w0].z;
        tmp_31 = f(tmp_30, 1w0);
        s[tmp_21].z = tmp_30;
        tmp_24 = (bit<32>)tmp_31;
        tmp_34 = s[(bit<32>)tmp_31].z;
        s[tmp_24].z = tmp_34;
=======
    bit<32> a;
    bit<32> tmp_7;
    bit<32> tmp_12;
    bit<32> tmp_13;
    bit<32> tmp_14;
    bit<1> tmp_15;
    bit<1> tmp_16;
    bit<32> tmp_17;
    bit<1> tmp_18;
    bit<1> tmp_19;
    bit<1> tmp_20;
    bit<1> tmp_21;
    bit<1> tmp_22;
    @name("act") action act_0() {
        a = 32w0;
        tmp_7 = 32w0;
        s[32w0].z = 1w1;
        tmp_12 = 32w1;
        tmp_13 = 32w1;
        s[32w1].z = 1w0;
        tmp_14 = 32w0;
        tmp_15 = s[32w0].z;
        tmp_19 = s[32w0].z;
        tmp_20 = f(tmp_19, 1w0);
        tmp_15 = tmp_19;
        tmp_16 = tmp_20;
        s[tmp_14].z = tmp_19;
        a = (bit<32>)tmp_20;
        tmp_17 = (bit<32>)tmp_20;
        tmp_18 = s[(bit<32>)tmp_20].z;
        tmp_21 = s[(bit<32>)tmp_20].z;
        tmp_22 = f(tmp_21, 1w1);
        tmp_18 = tmp_21;
        s[tmp_17].z = tmp_21;
>>>>>>> Fixes for issues #355, #356; removed table parameters from the language and the associated tests
    }
    @name("tbl_act") table tbl_act {
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
