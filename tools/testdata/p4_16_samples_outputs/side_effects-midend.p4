extern bit<1> f(inout bit<1> x, in bit<1> y);
extern bit<1> g(inout bit<1> x);
header H {
    bit<1> z;
}

control c();
package top(c _c);
control my() {
    bit<1> a_0;
    H[2] s_0;
    bit<1> tmp;
    bit<1> tmp_1;
    bit<1> tmp_2;
    bit<1> tmp_4;
    bit<1> tmp_5;
    bit<1> tmp_7;
    bit<1> tmp_8;
    bit<1> tmp_9;
    bit<1> tmp_10;
    @hidden action act() {
        a_0 = 1w0;
        tmp = g(a_0);
        tmp_1 = f(a_0, tmp);
        a_0 = tmp_1;
        tmp_2 = g(a_0);
        tmp_4 = f(s_0[a_0].z, tmp_2);
        a_0 = tmp_4;
        tmp_5 = g(a_0);
        tmp_7 = s_0[tmp_5].z;
        tmp_8 = f(tmp_7, a_0);
        s_0[tmp_5].z = tmp_7;
        a_0 = tmp_8;
        tmp_9 = g(a_0);
        a_0 = tmp_9;
        tmp_10 = g(a_0[0:0]);
        a_0[0:0] = tmp_10;
        g(a_0);
    }
    @hidden table tbl_act {
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

