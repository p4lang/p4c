extern bit<1> f(inout bit<1> x, in bit<1> y);
extern bit<1> g(inout bit<1> x);
header H {
    bit<1> z;
}

control c();
package top(c _c);
control my() {
    @name("a") bit<1> a;
    @name("s") H[2] s;
    @name("tmp") bit<1> tmp_17;
    @name("tmp_0") bit<1> tmp_18;
    @name("tmp_1") bit<1> tmp_19;
    @name("tmp_2") bit<1> tmp_20;
    @name("tmp_3") bit<1> tmp_21;
    @name("tmp_4") bit<1> tmp_22;
    @name("tmp_5") bit<1> tmp_23;
    @name("tmp_6") bit<1> tmp_24;
    @name("tmp_7") bit<1> tmp_25;
    @name("tmp_8") bit<1> tmp_26;
    @name("tmp_9") bit<1> tmp_27;
    @name("tmp_10") bit<1> tmp_28;
    @name("tmp_11") bit<1> tmp_29;
    @name("tmp_12") bit<1> tmp_30;
    @name("tmp_13") bit<1> tmp_31;
    @name("tmp_14") bit<1> tmp_32;
    @name("tmp_15") bit<1> tmp_33;
    @name("tmp_16") bit<1> tmp_34;
    action act() {
        a = 1w0;
        tmp_17 = a;
        tmp_18 = g(a);
        tmp_19 = tmp_18;
        tmp_20 = f(tmp_17, tmp_19);
        a = tmp_17;
        a = tmp_20;
        tmp_21 = a;
        tmp_22 = s[tmp_21].z;
        tmp_23 = g(a);
        tmp_24 = tmp_23;
        tmp_25 = f(tmp_22, tmp_24);
        s[tmp_21].z = tmp_22;
        a = tmp_25;
        tmp_26 = g(a);
        tmp_27 = tmp_26;
        tmp_28 = s[tmp_27].z;
        tmp_29 = a;
        tmp_30 = f(tmp_28, tmp_29);
        s[tmp_27].z = tmp_28;
        a = tmp_30;
        tmp_31 = g(a);
        a = tmp_31;
        tmp_32 = g(a[0:0]);
        a[0:0] = tmp_32;
        tmp_33 = a;
        tmp_34 = g(a);
        s[tmp_33].z = tmp_34;
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
