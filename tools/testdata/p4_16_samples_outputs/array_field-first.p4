header H {
    bit<1> z;
}

extern bit<32> f(inout bit<1> x, in bit<1> b);
control c(out H[2] h);
package top(c _c);
control my(out H[2] s) {
    apply {
        bit<32> a = 32w0;
        s[a].z = 1w1;
        s[a + 32w1].z = 1w0;
        a = f(s[a].z, 1w0);
        a = f(s[a].z, 1w1);
    }
}

top(my()) main;

