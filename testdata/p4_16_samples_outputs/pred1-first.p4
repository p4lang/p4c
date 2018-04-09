#include <core.p4>
#include <v1model.p4>

control empty();
package top(empty e);
control Ing() {
    bool b;
    bit<32> a = 32w2;
    action cond() {
        b = true;
        if (b) 
            a = 32w5;
        else 
            if (b && a == 32w5) 
                a = 32w10;
            else 
                a = 32w20;
    }
    apply {
        cond();
    }
}

top(Ing()) main;

