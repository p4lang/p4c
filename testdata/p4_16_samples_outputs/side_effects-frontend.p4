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
    bit<1> tmp_0;
    bit<1> tmp_1;
    bit<1> tmp_2;
    bit<1> tmp_3;
    bit<1> tmp_4;
    bit<1> tmp_5;
    bit<1> tmp_6;
    bit<1> tmp_7;
    bit<1> tmp_8;
    bit<1> tmp_9;
    bit<1> tmp_10;
    bit<1> tmp_11;
    bit<1> tmp_12;
    bit<1> tmp_13;
    bit<1> tmp_14;
    bit<1> tmp_15;
    bit<1> tmp_16;
    apply {
        a_0 = 1w0;
        tmp = a_0;
        tmp_0 = g(a_0);
        tmp_1 = tmp_0;
        tmp_2 = f(tmp, tmp_1);
        a_0 = tmp_2;
        tmp_3 = a_0;
        tmp_4 = s_0[tmp_3].z;
        tmp_5 = g(a_0);
        tmp_6 = tmp_5;
        tmp_7 = f(tmp_4, tmp_6);
        s_0[tmp_3].z = tmp_4;
        a_0 = tmp_7;
        tmp_8 = g(a_0);
        tmp_9 = tmp_8;
        tmp_10 = s_0[tmp_9].z;
        tmp_11 = a_0;
        tmp_12 = f(tmp_10, tmp_11);
        s_0[tmp_9].z = tmp_10;
        a_0 = tmp_12;
        tmp_13 = g(a_0);
        a_0 = tmp_13;
        tmp_14 = g(a_0[0:0]);
        a_0[0:0] = tmp_14;
        tmp_15 = a_0;
        tmp_16 = g(a_0);
    }
}

top(my()) main;
