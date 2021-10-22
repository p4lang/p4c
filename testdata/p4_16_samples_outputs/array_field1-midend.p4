header H {
    bit<1> z;
}

extern bit<1> f(inout bit<1> x, in bit<1> b);
control c(out H[2] h);
package top(c _c);
control my(out H[2] s) {
    @name("my.tmp_12") bit<1> tmp_3;
    @name("my.tmp_13") bit<1> tmp_4;
    @name("my.tmp_15") bit<1> tmp_6;
    bit<32> hsiVar0;
    H hsVar4;
    @name("my.act") action act() {
        s[32w0].z = 1w1;
        s[32w1].z = 1w0;
        tmp_3 = s[32w0].z;
        tmp_4 = f(tmp_3, 1w0);
        s[32w0].z = tmp_3;
        hsiVar0 = (bit<32>)tmp_4;
        if (hsiVar0 == 32w0) {
            tmp_6 = s[32w0].z;
        } else if (hsiVar0 == 32w1) {
            tmp_6 = s[32w1].z;
        } else {
            s[32w1] = hsVar4;
            if (hsiVar0 >= 32w1) {
                tmp_6 = s[32w1].z;
            }
        }
        f(tmp_6, 1w1);
        hsiVar0 = (bit<32>)tmp_4;
        if (hsiVar0 == 32w0) {
            s[32w0].z = tmp_6;
        } else if (hsiVar0 == 32w1) {
            s[32w1].z = tmp_6;
        } else {
            s[32w1] = hsVar4;
            if (hsiVar0 >= 32w1) {
                s[32w1].z = tmp_6;
            }
        }
    }
    @name("my.tbl_act") table tbl_act_0 {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        tbl_act_0.apply();
    }
}

top(my()) main;

