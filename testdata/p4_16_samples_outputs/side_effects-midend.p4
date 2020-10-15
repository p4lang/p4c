extern bit<1> f(inout bit<1> x, in bit<1> y);
extern bit<1> g(inout bit<1> x);
header H {
    bit<1> z;
}

control c();
package top(c _c);
control my() {
    @name("my.a") bit<1> a_0;
    @name("my.s") H[2] s_0;
    @name("my.tmp") bit<1> tmp;
    @name("my.tmp_1") bit<1> tmp_1;
    @name("my.tmp_3") bit<1> tmp_3;
    @name("my.tmp_5") bit<1> tmp_5;
    @name("my.tmp_6") bit<1> tmp_6;
    @hidden action side_effects27() {
        a_0 = 1w0;
        tmp = g(a_0);
        a_0 = f(a_0, tmp);
        tmp_1 = g(a_0);
        a_0 = f(s_0[a_0].z, tmp_1);
        tmp_3 = g(a_0);
        tmp_5 = s_0[tmp_3].z;
        tmp_6 = f(tmp_5, a_0);
        s_0[tmp_3].z = tmp_5;
        a_0 = tmp_6;
        a_0 = g(a_0);
        a_0 = g(a_0);
        g(a_0);
    }
    @hidden table tbl_side_effects27 {
        actions = {
            side_effects27();
        }
        const default_action = side_effects27();
    }
    apply {
        tbl_side_effects27.apply();
    }
}

top(my()) main;

