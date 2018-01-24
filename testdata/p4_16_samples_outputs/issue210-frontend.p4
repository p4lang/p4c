#include <core.p4>

control Ing(out bit<32> a) {
    bool b;
    @name("Ing.cond") action cond_0() {
        if (b) 
            a = 32w5;
        else 
            a = 32w10;
    }
    apply {
        b = true;
        cond_0();
    }
}

control s(out bit<32> a);
package top(s e);
top(Ing()) main;

