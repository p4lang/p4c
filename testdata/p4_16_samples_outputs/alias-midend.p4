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

