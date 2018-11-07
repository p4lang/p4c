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
    @name("c.o") Overloaded() o_0;
    @hidden action act() {
        f();
        f(a = 32w2);
        f(b = 16w1);
        f(a = 32w1, b = 16w2);
        f(b = 16w2, a = 32w1);
        o_0.f();
        o_0.f(a = 32w2);
        o_0.f(b = 16w1);
        o_0.f(a = 32w1, b = 16w2);
        o_0.f(b = 16w2, a = 32w1);
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        tbl_act.apply();
    }
}

control proto();
package top(proto p);
top(p = c()) main;

