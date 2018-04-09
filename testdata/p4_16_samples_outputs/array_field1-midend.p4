header H {
    bit<1> z;
}

extern bit<1> f(inout bit<1> x, in bit<1> b);
control c(out H[2] h);
package top(c _c);
control my(out H[2] s) {
    bit<32> tmp_12;
    bit<1> tmp_13;
    bit<32> tmp_15;
    bit<1> tmp_16;
    bit<1> tmp_17;
    @name("my.act") action act_0() {
        s[32w0].z = 1w1;
        s[32w1].z = 1w0;
        tmp_12 = 32w0;
        tmp_13 = s[32w0].z;
        tmp_17 = f(tmp_13, 1w0);
        s[tmp_12].z = tmp_13;
        tmp_15 = (bit<32>)tmp_17;
        tmp_16 = s[(bit<32>)tmp_17].z;
        f(tmp_16, 1w1);
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

