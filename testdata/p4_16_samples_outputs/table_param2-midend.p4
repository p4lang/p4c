#include <core.p4>

control c(inout bit<32> arg) {
    @name("a") action a_0() {
    }
    @name("t") table t() {
        key = {
            arg: exact @name("x") ;
        }
        actions = {
            a_0();
        }
        default_action = a_0();
    }
    action act() {
        arg = 32w0;
    }
    action act_0() {
        arg = arg + 32w1;
    }
    table tbl_act() {
        actions = {
            act();
        }
        const default_action = act();
    }
    table tbl_act_0() {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    apply {
        if (t.apply().hit) 
            tbl_act.apply();
        else 
            tbl_act_0.apply();
    }
}

control proto(inout bit<32> arg);
package top(proto p);
top(c()) main;
