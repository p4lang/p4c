#include <core.p4>

control Ing(out bit<32> a) {
    bool b;
    action cond() {
        if (b) 
            a = 5;
        else 
            a = 10;
    }
    apply {
        b = true;
        cond();
    }
}

control s(out bit<32> a);
package top(s e);
top(Ing()) main;

