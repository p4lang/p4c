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
    @name("my.tmp_0") bit<1> tmp_0;
    @name("my.tmp_1") bit<1> tmp_1;
    @name("my.tmp_2") bit<1> tmp_2;
    @name("my.tmp_3") bit<1> tmp_3;
    @name("my.tmp_4") bit<1> tmp_4;
    @name("my.tmp_5") bit<1> tmp_5;
    @name("my.tmp_6") bit<1> tmp_6;
    apply {
        a_0 = 1w0;
        tmp = g(a_0);
        tmp_0 = tmp;
        a_0 = f(a_0, tmp_0);
        tmp_1 = g(a_0);
        tmp_2 = tmp_1;
        a_0 = f(s_0[a_0].z, tmp_2);
        tmp_3 = g(a_0);
        tmp_4 = tmp_3;
        tmp_5 = s_0[tmp_4].z;
        tmp_6 = f(tmp_5, a_0);
        s_0[tmp_4].z = tmp_5;
        a_0 = tmp_6;
        a_0 = g(a_0);
        a_0 = g(a_0);
        g(a_0);
    }
}

top(my()) main;

