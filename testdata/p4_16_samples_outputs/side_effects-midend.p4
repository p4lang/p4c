extern bit<1> f(inout bit<1> x, in bit<1> y);
extern bit<1> g(inout bit<1> x);
header H {
    bit<1> z;
}

control c<T>(inout T t);
package top<T>(c<T> _c);
control my(inout H[2] s) {
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
    @hidden action side_effects27() {
        a_0 = 1w0;
        tmp = 1w0;
        tmp_0 = g(a_0);
        tmp_1 = f(tmp, tmp_0);
        a_0 = tmp_1;
        tmp_3 = s[tmp_1].z;
        tmp_4 = g(a_0);
        tmp_5 = f(tmp_3, tmp_4);
        s[tmp_1].z = tmp_3;
        a_0 = tmp_5;
        tmp_6 = g(a_0);
        tmp_8 = s[tmp_6].z;
        tmp_10 = f(tmp_8, a_0);
        s[tmp_6].z = tmp_8;
        a_0 = tmp_10;
        a_0 = g(a_0);
        a_0 = g(a_0);
        s[a_0].z = g(a_0);
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

top<H[2]>(my()) main;

