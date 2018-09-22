#include <core.p4>

typedef bit<32> B32;
type bit<32> N32;
struct S {
    B32 b;
    N32 n;
}

header H {
    N32 field;
}

type N32 NN32;
control c(out B32 x) {
    N32 k;
    NN32 nn;
    table t {
        actions = {
            NoAction();
        }
        key = {
            k: exact @name("k") ;
        }
        default_action = NoAction();
    }
    apply {
        bit<32> b = 32w0;
        N32 n = (N32)32w1;
        N32 n1;
        S s;
        NN32 n5 = (NN32)(N32)32w5;
        n = (N32)b;
        nn = (NN32)n;
        k = n;
        x = (B32)n;
        n1 = (N32)32w1;
        if (n == n1) 
            x = 32w2;
        s.b = b;
        s.n = n;
        t.apply();
        if (s.b == (B32)s.n) 
            x = 32w3;
    }
}

control e(out B32 x);
package top(e _e);
top(c()) main;

