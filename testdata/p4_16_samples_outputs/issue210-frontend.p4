#include <core.p4>

control Ing(out bit<32> a) {
    bool b_0;
    @name("Ing.cond") action cond() {
        if (b_0) {
            a = 32w5;
        } else {
            a = 32w10;
        }
    }
    apply {
        b_0 = true;
        cond();
    }
}

control s(out bit<32> a);
package top(s e);
top(Ing()) main;

