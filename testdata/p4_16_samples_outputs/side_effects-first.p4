extern bit<1> f(inout bit<1> x, in bit<1> y);
extern bit<1> g(inout bit<1> x);
header H {
    bit<1> z;
}

control c();
package top(c _c);
control my() {
    apply {
        bit<1> a = 1w0;
        H[2] s;
        a = f(a, g(a));
        a = f(s[a].z, g(a));
        a = f(s[g(a)].z, a);
        a = g(a);
        a[0:0] = g(a[0:0]);
        s[a].z = g(a);
    }
}

top(my()) main;

