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
    bit<1> tmp_15;
    bit<1> tmp_16;
    bit<1> tmp_18;
    bit<1> tmp_19;
    bit<1> tmp_20;
    bit<1> tmp_22;
    bit<1> tmp_23;
    bit<1> tmp_25;
    bit<1> tmp_27;
    bit<1> tmp_28;
    bit<1> tmp_29;
    @hidden action act() {
        a = 1w0;
        tmp_15 = 1w0;
        tmp_16 = g(a);
        tmp_18 = f(tmp_15, tmp_16);
        a = tmp_18;
        tmp_19 = s[tmp_18].z;
        tmp_20 = g(a);
        tmp_22 = f(tmp_19, tmp_20);
        s[a].z = tmp_19;
        a = tmp_22;
        tmp_23 = g(a);
        tmp_25 = s[tmp_23].z;
        tmp_27 = f(tmp_25, a);
        s[tmp_23].z = tmp_25;
        a = tmp_27;
        tmp_28 = g(a);
        a = tmp_28;
        tmp_29 = g(a[0:0]);
        a[0:0] = tmp_29;
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
