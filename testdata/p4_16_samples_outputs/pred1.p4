#include <core.p4>
#include <v1model.p4>

control empty();
package top(empty e);
control Ing() {
    bool b;
    bit<32> a = 2;
    action cond() {
        b = true;
        if (b) {
            a = 5;
        } else {
            if (b && a == 5) {
                a = 10;
            } else {
                a = 20;
            }
        }
    }
    apply {
        cond();
    }
}

top(Ing()) main;

