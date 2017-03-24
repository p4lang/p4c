header H {
    bit<1> z;
}

extern bit<1> f(inout bit<1> x, in bit<1> b);
control c(out H[2] h);
package top(c _c);
control my(out H[2] s) {
    bit<1> tmp_1;
    action act() {
        s[32w0].z = 1w1;
        s[32w1].z = 1w0;
        tmp_1 = f(s[32w0].z, 1w0);
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
