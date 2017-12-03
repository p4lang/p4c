extern bit<1> f(inout bit<1> x, in bit<1> y);
extern bit<1> g(inout bit<1> x);
header H {
    bit<1> z;
}

control c();
package top(c _c);
control my() {
    bit<1> a;
    H[2] s;
    bit<1> tmp_12;
    bit<1> tmp_14;
    bit<1> tmp_15;
    bit<1> tmp_17;
    bit<1> tmp_18;
    bit<1> tmp_20;
    bit<1> tmp_21;
    bit<1> tmp_22;
    bit<1> tmp_23;
    @hidden action act() {
        a = 1w0;
        tmp_12 = g(a);
        tmp_14 = f(a, tmp_12);
        a = tmp_14;
        tmp_15 = g(a);
        tmp_17 = f(s[a].z, tmp_15);
        a = tmp_17;
        tmp_18 = g(a);
        tmp_20 = s[tmp_18].z;
        tmp_21 = f(tmp_20, a);
        s[tmp_18].z = tmp_20;
        a = tmp_21;
        tmp_22 = g(a);
        a = tmp_22;
        tmp_23 = g(a[0:0]);
        a[0:0] = tmp_23;
        g(a);
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

