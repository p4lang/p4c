#include <core.p4>
#include <v1model.p4>

control empty();
package top(empty e);
control Ing() {
    bool b_0;
    bit<32> a_0;
    @name("Ing.cond") action cond() {
        b_0 = true;
    }
    apply {
        a_0 = 32w2;
        cond();
    }
}

top(Ing()) main;

