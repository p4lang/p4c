header H {
    bit<1> z;
}

extern bit<1> f(inout bit<1> x, in bit<1> b);
control c(out H[2] h);
package top(c _c);
control my(out H[2] s) {
    bit<1> tmp_3;
    bit<1> tmp_6;
    bit<1> tmp_8;
    @name("my.act") action act() {
        s[32w0].z = 1w1;
        s[32w1].z = 1w0;
        tmp_3 = s[32w0].z;
        tmp_8 = f(tmp_3, 1w0);
        s[32w0].z = tmp_3;
        tmp_6 = s[(bit<32>)tmp_8].z;
        f(tmp_6, 1w1);
        s[(bit<32>)tmp_8].z = tmp_6;
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

