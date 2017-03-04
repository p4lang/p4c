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
    bit<1> tmp_17;
    bit<1> tmp_18;
    bit<1> tmp_20;
    bit<1> tmp_22;
    bit<1> tmp_23;
    bit<1> tmp_25;
    bit<1> tmp_26;
    bit<1> tmp_28;
    bit<1> tmp_30;
    bit<1> tmp_31;
    bit<1> tmp_32;
    action act() {
        a = 1w0;
        tmp_17 = 1w0;
        tmp_18 = g(a);
        tmp_20 = f(tmp_17, tmp_18);
        a = tmp_20;
        tmp_22 = s[tmp_20].z;
        tmp_23 = g(a);
        tmp_25 = f(tmp_22, tmp_23);
        s[tmp_20].z = tmp_22;
        a = tmp_25;
        tmp_26 = g(a);
        tmp_28 = s[tmp_26].z;
        tmp_30 = f(tmp_28, a);
        s[tmp_26].z = tmp_28;
        a = tmp_30;
        tmp_31 = g(a);
        a = tmp_31;
        tmp_32 = g(a[0:0]);
        a[0:0] = tmp_32;
    }
    table tbl_act() {
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
