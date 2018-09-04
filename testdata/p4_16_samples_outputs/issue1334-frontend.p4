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

control c() {
    bit<32> z;
    @name("c.o") Overloaded() o;
    apply {
        f();
        f(a = 32w2);
        f(b = 16w1);
        f(a = 32w1, b = 16w2);
        f(b = 16w2, a = 32w1);
        o.f();
        o.f(a = 32w2);
        o.f(b = 16w1);
        o.f(a = 32w1, b = 16w2);
        o.f(b = 16w2, a = 32w1);
        z = 32w4294967294;
    }
}

control proto();
package top(proto p);
top(p = c()) main;

