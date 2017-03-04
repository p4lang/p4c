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
    table tbl_act() {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        if (t.apply().hit) 
            tbl_act.apply();
        else 
            ;
    }
}

control proto(inout bit<32> arg);
package top(proto p);
top(c()) main;
