struct T {
    bit<32> b;
}

header H {
    bit<32> b;
}

struct S {
    H[3]    h;
    bit<32> a;
    T       t;
}

extern E {
    E();
    void set1(out bit<32> x, out S y);
    void set2(out bit<32> x, out bit<32> y);
    void set3(out H h, out bit<32> y);
    void set4(out S x, out bit<32> y);
}

control c() {
    E() e;
    apply {
        S s;
        e.set1(s.a, s);
        e.set2(s.a, s.a);
        e.set2(s.a, s.t.b);
        e.set2(s.h[1].b, s.h[2].b);
        e.set3(s.h[1], s.h[1].b);
        e.set4(s, s.h[2].b);
    }
}

