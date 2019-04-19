header H {
    bit<32> a;
    varbit<32> b;
}

struct S {
    bit<32> a;
    H h;
}

control c(out bit x) {
    varbit<32> a;
    varbit<32> b;
    H h1;
    H h2;
    S s1;
    S s2;
    H[2] a1;
    H[2] a2;
    H[3] a3;

    apply {
        if (a == h1) {
            x = 1;
        } else if (h1 == s2) {
            x = 1;
        } else if (s1 == a2) {
            x = 1;
        } else if (a1 == h1.a) {
            x = 1;
        } else if (a == h1.a) {
            x = 1;
        } else if (a1 == a3) {
            x = 1;
        } else {
            x = 0;
        }
    }
}

control ctrl(out bit x);
package top(ctrl _c);

top(c()) main;
