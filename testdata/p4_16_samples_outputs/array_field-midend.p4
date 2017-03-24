header H {
    bit<1> z;
}

extern bit<1> f(inout bit<1> x, in bit<1> b);
control c(out H[2] h);
package top(c _c);
control my(out H[2] s) {
    bit<32> a;
    bit<1> tmp_3;
    bit<1> tmp_4;
    bit<1> tmp_5;
    bit<1> tmp_6;
    action act() {
        a = 32w0;
        s[32w0].z = 1w1;
        s[32w1].z = 1w0;
        tmp_3 = s[32w0].z;
        tmp_4 = f(tmp_3, 1w0);
        s[a].z = tmp_3;
        a = (bit<32>)tmp_4;
        tmp_5 = s[(bit<32>)tmp_4].z;
        tmp_6 = f(tmp_5, 1w1);
        s[a].z = tmp_5;
    }
    table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        tbl_act.apply();
    }
}

top(my()) main;
