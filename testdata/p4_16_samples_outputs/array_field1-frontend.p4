header H {
    bit<1> z;
}

extern bit<1> f(inout bit<1> x, in bit<1> b);
control c(out H[2] h);
package top(c _c);
control my(out H[2] s) {
    bit<32> a_0;
    bit<32> tmp;
    bit<32> tmp_0;
    bit<32> tmp_1;
    bit<32> tmp_2;
    bit<1> tmp_3;
    bit<1> tmp_4;
    bit<32> tmp_5;
    bit<1> tmp_6;
    bit<32> tmp_8;
    bit<32> tmp_9;
    bit<32> tmp_10;
    bit<32> tmp_11;
    bit<1> tmp_12;
    bit<1> tmp_13;
    bit<32> tmp_14;
    bit<32> tmp_15;
    bit<1> tmp_16;
    bit<1> tmp_17;
    bit<32> tmp_18;
    @name("act") action act_0() {
        a_0 = 32w0;
        tmp = a_0;
        tmp_8 = tmp;
        s[tmp_8].z = 1w1;
        tmp_9 = a_0 + 32w1;
        tmp_0 = tmp_9;
        tmp_1 = tmp_0;
        tmp_10 = tmp_1;
        s[tmp_10].z = 1w0;
        tmp_2 = a_0;
        tmp_11 = tmp_2;
        tmp_3 = s[tmp_11].z;
        tmp_12 = tmp_3;
        tmp_13 = f(tmp_12, 1w0);
        tmp_3 = tmp_12;
        tmp_4 = tmp_13;
        tmp_14 = tmp_2;
        s[tmp_14].z = tmp_3;
        a_0 = (bit<32>)tmp_4;
        tmp_5 = a_0;
        tmp_15 = tmp_5;
        tmp_6 = s[tmp_15].z;
        tmp_16 = tmp_6;
        tmp_17 = f(tmp_16, 1w1);
        tmp_6 = tmp_16;
        tmp_18 = tmp_5;
        s[tmp_18].z = tmp_6;
    }
    @name("tbl_act") table tbl_act_0() {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    apply {
        tbl_act_0.apply();
    }
}

top(my()) main;
