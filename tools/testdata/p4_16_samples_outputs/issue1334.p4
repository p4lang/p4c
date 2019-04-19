extern void f();
extern void f(bit<32> a);
extern void f(bit<16> b);
extern void f(bit<32> a, bit<16> b);
extern Overloaded {
    Overloaded();
    void f();
    void f(bit<32> a);
    void f(bit<16> b);
    void f(bit<32> a, bit<16> b);
}

control d(out bit<32> x)(bit<32> a) {
    apply {
        x = a;
    }
}

control d(out bit<32> x)(bit<32> b) {
    apply {
        x = -b;
    }
}

control c() {
    Overloaded() o;
    d(b = 2) dinst;
    apply {
        f();
        f(a = 2);
        f(b = 1);
        f(a = 1, b = 2);
        f(b = 2, a = 1);
        o.f();
        o.f(a = 2);
        o.f(b = 1);
        o.f(a = 1, b = 2);
        o.f(b = 2, a = 1);
        bit<32> z;
        dinst.apply(z);
    }
}

control proto();
package top(proto p);
package top(proto p1, proto p2);
top(p = c()) main;

