#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

control empty();
package top(empty e);
control Ing() {
    bool b_0;
    @name("Ing.cond") action cond() {
        b_0 = true;
    }
    apply {
        cond();
    }
}

top(Ing()) main;

