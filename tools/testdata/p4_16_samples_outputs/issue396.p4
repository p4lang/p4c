header H {
    bit<32> x;
}

struct S {
    H h;
}

control c(out bool b);
package top(c _c);
control e(in H h, out bool valid) {
    apply {
        valid = h.isValid();
    }
}

control d(out bool b) {
    e() einst;
    apply {
        H h;
        H h1;
        H[2] h3;
        h = { 0 };
        S s;
        S s1;
        s = { { 0 } };
        s1.h = { 0 };
        h3[0] = { 0 };
        h3[1] = { 1 };
        bool eout;
        einst.apply({ 0 }, eout);
        b = h.isValid() && eout && h3[1].isValid() && s1.h.isValid();
    }
}

top(d()) main;

