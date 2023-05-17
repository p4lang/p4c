header H {
    bit<1> z;
}

extern bit<32> f(inout bit<1> x, in bit<1> b);
control c(out H[2] h);
package top(c _c);
control my(out H[2] s) {
    @name("my.a") bit<32> a_0;
    @name("my.tmp") bit<32> tmp;
    @name("my.tmp_0") bit<32> tmp_0;
    apply {
        a_0 = 32w0;
        s[a_0].z = 1w1;
        s[a_0 + 32w1].z = 1w0;
        tmp = a_0;
        a_0 = f(s[tmp].z, 1w0);
        tmp_0 = a_0;
        a_0 = f(s[tmp_0].z, 1w1);
    }
}

top(my()) main;
