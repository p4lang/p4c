header H {
    bit<1> z;
}

extern bit<1> f(inout bit<1> x, in bit<1> b);
control c(out H[2] h);
package top(c _c);
control my(out H[2] s) {
    bit<32> a;
    bit<32> tmp_7;
    bit<32> tmp_10;
    bit<32> tmp_11;
    bit<32> tmp_12;
    bit<1> tmp_13;
    bit<1> tmp_14;
    bit<32> tmp_15;
    bit<1> tmp_16;
    bit<1> tmp_17;
    bit<1> tmp_18;
    @name("my.act") action act_0() {
        a = 32w0;
        tmp_7 = a;
        s[tmp_7].z = 1w1;
        tmp_10 = a + 32w1;
        tmp_11 = tmp_10;
        s[tmp_11].z = 1w0;
        tmp_12 = a;
        tmp_13 = s[tmp_12].z;
        tmp_17 = f(tmp_13, 1w0);
        tmp_14 = tmp_17;
        s[tmp_12].z = tmp_13;
        a = (bit<32>)tmp_14;
        tmp_15 = a;
        tmp_16 = s[tmp_15].z;
        tmp_18 = f(tmp_16, 1w1);
        s[tmp_15].z = tmp_16;
    }
    @name("my.tbl_act") table tbl_act {
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

