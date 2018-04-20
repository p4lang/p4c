typedef bit<32> B32;
newtype bit<32> N32;
struct S {
    B32 b;
    N32 n;
}

control c(out B32 x) {
    bit<32> b_1;
    N32 n_1;
    N32 n1;
    S s;
    apply {
        b_1 = 32w0;
        n_1 = (N32)b_1;
        x = (B32)n_1;
        n1 = (N32)32w1;
        if (n_1 == n1) 
            x = 32w2;
        s.b = b_1;
        s.n = n_1;
        if (s.b == (B32)s.n) 
            x = 32w3;
    }
}

control e(out B32 x);
package top(e _e);
top(c()) main;

