header H {
    bit<1> z;
}

extern bit<32> f(inout bit<1> x, in bit<1> b);
control c(out H[2] h);
package top(c _c);
control my(out H[2] s) {
    apply {
        bit<32> a = 0;
        s[a].z = 1;
        s[a + 1].z = 0;
        a = f(s[a].z, 0);
        a = f(s[a].z, 1);
    }
}

top(my()) main;

