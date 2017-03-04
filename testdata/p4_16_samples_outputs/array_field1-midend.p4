header H {
    bit<1> z;
}

extern bit<1> f(inout bit<1> x, in bit<1> b);
control c(out H[2] h);
package top(c _c);
control my(out H[2] s) {
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
