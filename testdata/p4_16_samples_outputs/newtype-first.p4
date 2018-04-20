typedef bit<32> B32;
newtype bit<32> N32;
struct S {
    B32 b;
    N32 n;
}

control c(out B32 x) {
    apply {
        bit<32> b = 32w0;
        N32 n;
        N32 n1;
        S s;
        n = (N32)b;
        x = (B32)n;
        n1 = (N32)32w1;
        if (n == n1) 
            x = 32w2;
        s.b = b;
        s.n = n;
        if (s.b == (B32)s.n) 
            x = 32w3;
    }
}

control e(out B32 x);
package top(e _e);
top(c()) main;

