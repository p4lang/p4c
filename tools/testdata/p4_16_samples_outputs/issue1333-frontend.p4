#include <core.p4>

extern void f(bit<32> a=0, bit<32> b);
extern E {
    E(bit<32> x=0);
    void f(in bit<16> z=2);
}

control ctrl();
package top(ctrl _c);
control c_0() {
    @name("c_0.e") E(x = 32w0) e_0;
    apply {
        f(a = 32w0, b = 32w4);
        e_0.f(z = 16w2);
    }
}

top(c_0()) main;

