header H {
    bit<1> z;
}

extern bit<1> f(inout bit<1> x, in bit<1> b);
control c(out H[2] h);
package top(c _c);
control my(out H[2] s) {
    bit<32> a_0;
    bit<1> tmp;
    bit<1> tmp_0;
    apply {
        a_0 = 32w0;
        s[a_0].z = 1w1;
        s[a_0 + 32w1].z = 1w0;
        tmp = f(s[a_0].z, 1w0);
        a_0 = (bit<32>)tmp;
        tmp_0 = f(s[a_0].z, 1w1);
    }
}

top(my()) main;
