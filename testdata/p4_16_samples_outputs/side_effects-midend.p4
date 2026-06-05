extern bit<1> f(inout bit<1> x, in bit<1> y);
extern bit<1> g(inout bit<1> x);
header H {
    bit<1> z;
}

control c<T>(inout T t);
package top<T>(c<T> _c);
control my(inout H[2] s) {
    bit<1> hsVar;
    @name("my.a") bit<1> a_0;
    @name("my.tmp") bit<1> tmp;
    @name("my.tmp_0") bit<1> tmp_0;
    @name("my.tmp_1") bit<1> tmp_1;
    @name("my.tmp_3") bit<1> tmp_3;
    @name("my.tmp_4") bit<1> tmp_4;
    @name("my.tmp_5") bit<1> tmp_5;
    @name("my.tmp_6") bit<1> tmp_6;
    @name("my.tmp_8") bit<1> tmp_8;
    @name("my.tmp_10") bit<1> tmp_10;
    @hidden action side_effects21() {
        tmp_3 = s[1w0].z;
    }
    @hidden action side_effects21_0() {
        tmp_3 = s[1w1].z;
    }
    @hidden action side_effects21_1() {
        tmp_3 = hsVar;
    }
    @hidden action side_effects18() {
        a_0 = 1w0;
        tmp = 1w0;
        tmp_0 = g(a_0);
        tmp_1 = f(tmp, tmp_0);
        a_0 = tmp_1;
    }
    @hidden action act() {
        s[1w0].z = tmp_3;
    }
    @hidden action act_0() {
        s[1w1].z = tmp_3;
    }
    @hidden action act_1() {
        tmp_4 = g(a_0);
        tmp_5 = f(tmp_3, tmp_4);
    }
    @hidden action side_effects22() {
        tmp_8 = s[1w0].z;
    }
    @hidden action side_effects22_0() {
        tmp_8 = s[1w1].z;
    }
    @hidden action side_effects22_1() {
        tmp_8 = hsVar;
    }
    @hidden action side_effects21_2() {
        a_0 = tmp_5;
        tmp_6 = g(a_0);
    }
    @hidden action act_2() {
        s[1w0].z = tmp_8;
    }
    @hidden action act_3() {
        s[1w1].z = tmp_8;
    }
    @hidden action act_4() {
        tmp_10 = f(tmp_8, a_0);
    }
    @hidden action side_effects25() {
        s[1w0].z = g(a_0);
    }
    @hidden action side_effects25_0() {
        s[1w1].z = g(a_0);
    }
    @hidden action side_effects22_2() {
        a_0 = tmp_10;
        a_0 = g(a_0);
        a_0 = g(a_0);
    }
    @hidden table tbl_side_effects18 {
        actions = {
            side_effects18();
        }
        const default_action = side_effects18();
    }
    @hidden table tbl_side_effects21 {
        actions = {
            side_effects21();
        }
        const default_action = side_effects21();
    }
    @hidden table tbl_side_effects21_0 {
        actions = {
            side_effects21_0();
        }
        const default_action = side_effects21_0();
    }
    @hidden table tbl_side_effects21_1 {
        actions = {
            side_effects21_1();
        }
        const default_action = side_effects21_1();
    }
    @hidden table tbl_act {
        actions = {
            act_1();
        }
        const default_action = act_1();
    }
    @hidden table tbl_act_0 {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_act_1 {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    @hidden table tbl_side_effects21_2 {
        actions = {
            side_effects21_2();
        }
        const default_action = side_effects21_2();
    }
    @hidden table tbl_side_effects22 {
        actions = {
            side_effects22();
        }
        const default_action = side_effects22();
    }
    @hidden table tbl_side_effects22_0 {
        actions = {
            side_effects22_0();
        }
        const default_action = side_effects22_0();
    }
    @hidden table tbl_side_effects22_1 {
        actions = {
            side_effects22_1();
        }
        const default_action = side_effects22_1();
    }
    @hidden table tbl_act_2 {
        actions = {
            act_4();
        }
        const default_action = act_4();
    }
    @hidden table tbl_act_3 {
        actions = {
            act_2();
        }
        const default_action = act_2();
    }
    @hidden table tbl_act_4 {
        actions = {
            act_3();
        }
        const default_action = act_3();
    }
    @hidden table tbl_side_effects22_2 {
        actions = {
            side_effects22_2();
        }
        const default_action = side_effects22_2();
    }
    @hidden table tbl_side_effects25 {
        actions = {
            side_effects25();
        }
        const default_action = side_effects25();
    }
    @hidden table tbl_side_effects25_0 {
        actions = {
            side_effects25_0();
        }
        const default_action = side_effects25_0();
    }
    apply {
        tbl_side_effects18.apply();
        if (tmp_1 == 1w0) {
            tbl_side_effects21.apply();
        } else if (tmp_1 == 1w1) {
            tbl_side_effects21_0.apply();
        } else if (tmp_1 >= 1w1) {
            tbl_side_effects21_1.apply();
        }
        tbl_act.apply();
        if (tmp_1 == 1w0) {
            tbl_act_0.apply();
        } else if (tmp_1 == 1w1) {
            tbl_act_1.apply();
        }
        tbl_side_effects21_2.apply();
        if (tmp_6 == 1w0) {
            tbl_side_effects22.apply();
        } else if (tmp_6 == 1w1) {
            tbl_side_effects22_0.apply();
        } else if (tmp_6 >= 1w1) {
            tbl_side_effects22_1.apply();
        }
        tbl_act_2.apply();
        if (tmp_6 == 1w0) {
            tbl_act_3.apply();
        } else if (tmp_6 == 1w1) {
            tbl_act_4.apply();
        }
        tbl_side_effects22_2.apply();
        if (a_0 == 1w0) {
            tbl_side_effects25.apply();
        } else if (a_0 == 1w1) {
            tbl_side_effects25_0.apply();
        }
    }
}

top<H[2]>(my()) main;
