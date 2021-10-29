extern bit<1> f(inout bit<1> x, in bit<1> y);
extern bit<1> g(inout bit<1> x);
header H {
    bit<1> z;
}

control c<T>(inout T t);
package top<T>(c<T> _c);
control my(inout H[2] s) {
    H hsVar0;
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
    @hidden action act() {
        tmp_3 = s[1w0].z;
    }
    @hidden action act_0() {
        tmp_3 = s[1w1].z;
    }
    @hidden action act_1() {
        tmp_3 = s[1w1].z;
    }
    @hidden action side_effects30() {
        s[1w0] = hsVar0;
        s[1w1] = hsVar0;
    }
    @hidden action side_effects27() {
        a_0 = 1w0;
        tmp = 1w0;
        tmp_0 = g(a_0);
        tmp_1 = f(tmp, tmp_0);
        a_0 = tmp_1;
    }
    @hidden action act_2() {
        s[1w0].z = tmp_3;
    }
    @hidden action act_3() {
        s[1w1].z = tmp_3;
    }
    @hidden action act_4() {
        s[1w1].z = tmp_3;
    }
    @hidden action side_effects30_0() {
        s[1w0] = hsVar0;
        s[1w1] = hsVar0;
    }
    @hidden action act_5() {
        tmp_4 = g(a_0);
        tmp_5 = f(tmp_3, tmp_4);
    }
    @hidden action act_6() {
        tmp_8 = s[1w0].z;
    }
    @hidden action act_7() {
        tmp_8 = s[1w1].z;
    }
    @hidden action act_8() {
        tmp_8 = s[1w1].z;
    }
    @hidden action side_effects31() {
        s[1w0] = hsVar0;
        s[1w1] = hsVar0;
    }
    @hidden action side_effects30_1() {
        a_0 = tmp_5;
        tmp_6 = g(a_0);
    }
    @hidden action act_9() {
        s[1w0].z = tmp_8;
    }
    @hidden action act_10() {
        s[1w1].z = tmp_8;
    }
    @hidden action act_11() {
        s[1w1].z = tmp_8;
    }
    @hidden action side_effects31_0() {
        s[1w0] = hsVar0;
        s[1w1] = hsVar0;
    }
    @hidden action act_12() {
        tmp_10 = f(tmp_8, a_0);
    }
    @hidden action side_effects34() {
        s[1w0].z = g(a_0);
    }
    @hidden action side_effects34_0() {
        s[1w1].z = g(a_0);
    }
    @hidden action side_effects34_1() {
        s[1w1].z = g(a_0);
    }
    @hidden action side_effects34_2() {
        s[1w0] = hsVar0;
        s[1w1] = hsVar0;
    }
    @hidden action side_effects31_1() {
        a_0 = tmp_10;
        a_0 = g(a_0);
        a_0 = g(a_0);
    }
    @hidden table tbl_side_effects27 {
        actions = {
            side_effects27();
        }
        const default_action = side_effects27();
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_act_0 {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    @hidden table tbl_side_effects30 {
        actions = {
            side_effects30();
        }
        const default_action = side_effects30();
    }
    @hidden table tbl_act_1 {
        actions = {
            act_1();
        }
        const default_action = act_1();
    }
    @hidden table tbl_act_2 {
        actions = {
            act_5();
        }
        const default_action = act_5();
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
    @hidden table tbl_side_effects30_0 {
        actions = {
            side_effects30_0();
        }
        const default_action = side_effects30_0();
    }
    @hidden table tbl_act_5 {
        actions = {
            act_4();
        }
        const default_action = act_4();
    }
    @hidden table tbl_side_effects30_1 {
        actions = {
            side_effects30_1();
        }
        const default_action = side_effects30_1();
    }
    @hidden table tbl_act_6 {
        actions = {
            act_6();
        }
        const default_action = act_6();
    }
    @hidden table tbl_act_7 {
        actions = {
            act_7();
        }
        const default_action = act_7();
    }
    @hidden table tbl_side_effects31 {
        actions = {
            side_effects31();
        }
        const default_action = side_effects31();
    }
    @hidden table tbl_act_8 {
        actions = {
            act_8();
        }
        const default_action = act_8();
    }
    @hidden table tbl_act_9 {
        actions = {
            act_12();
        }
        const default_action = act_12();
    }
    @hidden table tbl_act_10 {
        actions = {
            act_9();
        }
        const default_action = act_9();
    }
    @hidden table tbl_act_11 {
        actions = {
            act_10();
        }
        const default_action = act_10();
    }
    @hidden table tbl_side_effects31_0 {
        actions = {
            side_effects31_0();
        }
        const default_action = side_effects31_0();
    }
    @hidden table tbl_act_12 {
        actions = {
            act_11();
        }
        const default_action = act_11();
    }
    @hidden table tbl_side_effects31_1 {
        actions = {
            side_effects31_1();
        }
        const default_action = side_effects31_1();
    }
    @hidden table tbl_side_effects34 {
        actions = {
            side_effects34();
        }
        const default_action = side_effects34();
    }
    @hidden table tbl_side_effects34_0 {
        actions = {
            side_effects34_0();
        }
        const default_action = side_effects34_0();
    }
    @hidden table tbl_side_effects34_1 {
        actions = {
            side_effects34_2();
        }
        const default_action = side_effects34_2();
    }
    @hidden table tbl_side_effects34_2 {
        actions = {
            side_effects34_1();
        }
        const default_action = side_effects34_1();
    }
    apply {
        tbl_side_effects27.apply();
        if (tmp_1 == 1w0) {
            tbl_act.apply();
        } else if (tmp_1 == 1w1) {
            tbl_act_0.apply();
        } else {
            tbl_side_effects30.apply();
            if (tmp_1 >= 1w1) {
                tbl_act_1.apply();
            }
        }
        tbl_act_2.apply();
        if (tmp_1 == 1w0) {
            tbl_act_3.apply();
        } else if (tmp_1 == 1w1) {
            tbl_act_4.apply();
        } else {
            tbl_side_effects30_0.apply();
            if (tmp_1 >= 1w1) {
                tbl_act_5.apply();
            }
        }
        tbl_side_effects30_1.apply();
        if (tmp_6 == 1w0) {
            tbl_act_6.apply();
        } else if (tmp_6 == 1w1) {
            tbl_act_7.apply();
        } else {
            tbl_side_effects31.apply();
            if (tmp_6 >= 1w1) {
                tbl_act_8.apply();
            }
        }
        tbl_act_9.apply();
        if (tmp_6 == 1w0) {
            tbl_act_10.apply();
        } else if (tmp_6 == 1w1) {
            tbl_act_11.apply();
        } else {
            tbl_side_effects31_0.apply();
            if (tmp_6 >= 1w1) {
                tbl_act_12.apply();
            }
        }
        tbl_side_effects31_1.apply();
        if (a_0 == 1w0) {
            tbl_side_effects34.apply();
        } else if (a_0 == 1w1) {
            tbl_side_effects34_0.apply();
        } else {
            tbl_side_effects34_1.apply();
            if (a_0 >= 1w1) {
                tbl_side_effects34_2.apply();
            }
        }
    }
}

top<H[2]>(my()) main;

