#include <core.p4>

control empty();
package top(empty e);
control Ing() {
    @name("Ing.cond") action cond_0() {
    }
    @name("Ing.tbl_cond") table tbl_cond_0 {
        actions = {
            cond_0();
        }
        const default_action = cond_0();
    }
    apply {
        tbl_cond_0.apply();
    }
}

top(Ing()) main;
