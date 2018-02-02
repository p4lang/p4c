#include <core.p4>
#include <v1model.p4>

control empty();
package top(empty e);
control Ing() {
    bool b;
    bit<32> a;
    @name("Ing.cond") action cond_0() {
        b = true;
    }
    apply {
        a = 32w2;
        cond_0();
    }
}

top(Ing()) main;

