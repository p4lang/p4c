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
    apply {
        a_0 = 1w0;
        tmp = a_0;
        tmp_0 = g(a_0);
        tmp_1 = tmp_0;
        tmp_2 = f(tmp, tmp_1);
        a_0 = tmp_2;
        tmp_3 = s_0[a_0].z;
        tmp_4 = g(a_0);
        tmp_5 = tmp_4;
        tmp_6 = f(tmp_3, tmp_5);
        s_0[a_0].z = tmp_3;
        a_0 = tmp_6;
        tmp_7 = g(a_0);
        tmp_8 = tmp_7;
        tmp_9 = s_0[tmp_8].z;
        tmp_10 = a_0;
        tmp_11 = f(tmp_9, tmp_10);
        s_0[tmp_8].z = tmp_9;
        a_0 = tmp_11;
        tmp_12 = g(a_0);
        a_0 = tmp_12;
        tmp_13 = g(a_0[0:0]);
        a_0[0:0] = tmp_13;
        tmp_14 = g(a_0);
    }
}

top(my()) main;
