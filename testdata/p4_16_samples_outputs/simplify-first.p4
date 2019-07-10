#include <core.p4>

control c(out bool x) {
    table t1 {
        key = {
            x: exact @name("x") ;
        }
        actions = {
            NoAction();
        }
        default_action = NoAction();
    }
    table t2 {
        key = {
            x: exact @name("x") ;
        }
        actions = {
            NoAction();
        }
        default_action = NoAction();
    }
    apply {
        x = true;
        if (t1.apply().hit && t2.apply().hit) {
            x = false;
        }
    }
}

control proto(out bool x);
package top(proto p);
top(c()) main;

