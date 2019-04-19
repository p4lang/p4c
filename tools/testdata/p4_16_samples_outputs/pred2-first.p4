#include <core.p4>
#include <v1model.p4>

control empty();
package top(empty e);
control Ing() {
    bool b;
    bit<32> a;
    bool tmp_1;
    bool tmp_2;
    @name("cond") action cond_0() {
        b = true;
        tmp_1 = tmp_1;
        tmp_2 = tmp_2;
        tmp_1 = tmp_1;
    }
    table tbl_cond {
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

