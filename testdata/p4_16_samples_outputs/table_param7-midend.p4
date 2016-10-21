#include <core.p4>

control c(inout bit<32> arg) {
    bit<32> x_0;
    @name("a") action a_0() {
    }
    @name("t") table t() {
        key = {
            x_0: exact;
        }
        actions = {
            a_0();
        }
        default_action = a_0();
    }
    action act() {
        x_0 = arg;
    }
    action act_0() {
        x_0 = arg;
    }
    table tbl_act() {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    table tbl_act_0() {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        tbl_act.apply();
        if (t.apply().hit) {
            tbl_act_0.apply();
            t.apply();
        }
    }
}

control proto(inout bit<32> arg);
package top(proto p);
top(c()) main;
