#include <core.p4>
#include <v1model.p4>

control empty();
package top(empty e);
control Ing() {
    bool tmp;
    @name("cond") action cond() {
        tmp = tmp;
    }
    @name("tbl_cond") table tbl_cond_0 {
        actions = {
            cond();
        }
        const default_action = cond();
    }
    apply {
        tbl_cond_0.apply();
    }
}

top(Ing()) main;

