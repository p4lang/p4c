#include <core.p4>
#include <v1model.p4>

control empty();
package top(empty e);
control Ing() {
    bool b;
    @name("Ing.cond") action cond_0() {
        b = true;
    }
    apply {
        cond_0();
    }
}

top(Ing()) main;

