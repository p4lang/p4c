#include <core.p4>
#include <v1model.p4>

control empty();
package top(empty e);
control Ing() {
    bool tmp_0;
    @name("Ing.cond") action cond() {
        tmp_0 = tmp_0;
    }
    @name("Ing.tbl_cond") table tbl_cond {
        actions = {
            cond();
        }
        const default_action = cond();
    }
    apply {
        tbl_cond.apply();
    }
}

top(Ing()) main;

