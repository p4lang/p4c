#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

control empty();
package top(empty e);
control Ing() {
    @name("Ing.cond") action cond() {
    }
    @hidden table tbl_cond {
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

