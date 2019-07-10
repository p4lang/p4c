header H {
    bit<32>    a;
    varbit<32> b;
}

struct S {
    bit<32> a;
    H       h;
}

control c(out bit<1> x) {
    varbit<32> a_0;
    varbit<32> b_0;
    H h1_0;
    H h2_0;
    S s1_0;
    S s2_0;
    H[2] a1_0;
    H[2] a2_0;
    apply {
        if (a_0 == b_0) {
            x = 1w1;
        } else if (h1_0 == h2_0) {
            x = 1w1;
        } else if (s1_0 == s2_0) {
            x = 1w1;
        } else if (a1_0 == a2_0) {
            x = 1w1;
        } else {
            x = 1w0;
        }
    }
}

control ctrl(out bit<1> x);
package top(ctrl _c);
top(c()) main;

