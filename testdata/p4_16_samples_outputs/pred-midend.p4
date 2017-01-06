#include <core.p4>
#include <v1model.p4>

control empty();
package top(empty e);
control Ing() {
    bool b;
    @name("cond") action cond_0() {
        b = true;
    }
    table tbl_cond() {
        actions = {
            cond_0();
        }
        const default_action = cond_0();
    }
    apply {
        tbl_cond.apply();
    }
}

top(Ing()) main;
