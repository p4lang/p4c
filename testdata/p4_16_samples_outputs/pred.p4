#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

control empty();
package top(empty e);
control Ing() {
    bool b;
    bit<32> a;
    action cond() {
        b = true;
        if (b) {
            a = 5;
        } else {
            a = 10;
        }
    }
    apply {
        cond();
    }
}

top(Ing()) main;
