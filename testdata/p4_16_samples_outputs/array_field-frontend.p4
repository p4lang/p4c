header H {
    bit<1> z;
}

extern bit<1> f(inout bit<1> x, in bit<1> b);
control c(out H[2] h);
package top(c _c);
control my(out H[2] s) {
    bit<32> a_0;
    bit<32> tmp;
    bit<32> tmp_0;
    bit<32> tmp_1;
    bit<32> tmp_2;
    bit<1> tmp_3;
    bit<1> tmp_4;
    bit<1> tmp_5;
    bit<32> tmp_6;
    bit<1> tmp_7;
    bit<1> tmp_8;
    bit<1> tmp_9;
    apply {
        a_0 = 32w0;
        tmp = a_0;
        s[tmp].z = 1w1;
        tmp_0 = a_0 + 32w1;
        tmp_1 = tmp_0;
        s[tmp_1].z = 1w0;
        tmp_2 = a_0;
        tmp_3 = s[tmp_2].z;
        tmp_4 = 1w0;
        tmp_5 = f(tmp_3, tmp_4);
        s[tmp_2].z = tmp_3;
        a_0 = (bit<32>)tmp_5;
        tmp_6 = a_0;
        tmp_7 = s[tmp_6].z;
        tmp_8 = 1w1;
        tmp_9 = f(tmp_7, tmp_8);
        s[tmp_6].z = tmp_7;
    }
}

top(my()) main;
