extern bit<1> f(inout bit<1> x, in bit<1> y);
extern bit<1> g(inout bit<1> x);
header H {
    bit<1> z;
}

control c<T>(inout T t);
package top<T>(c<T> _c);
control my(inout H[2] s) {
    apply {
        bit<1> a = 1w0;
        a = f(a, g(a));
        a = f(s[a].z, g(a));
        a = f(s[g(a)].z, a);
        a = g(a);
        a = g(a);
        s[a].z = g(a);
    }
}

top<H[2]>(my()) main;
