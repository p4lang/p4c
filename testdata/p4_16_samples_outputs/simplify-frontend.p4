#include <core.p4>

control c(out bool x) {
    bool tmp;
    @name("t1") table t1_0() {
        key = {
            x: exact;
        }
        actions = {
            NoAction();
        }
        default_action = NoAction();
    }
    @name("t2") table t2_0() {
        key = {
            x: exact;
        }
        actions = {
            NoAction();
        }
        default_action = NoAction();
    }
    apply {
        x = true;
        if (!t1_0.apply().hit) 
            tmp = false;
        else 
            tmp = t2_0.apply().hit;
        if (tmp) 
            x = false;
    }
}

control proto(out bool x);
package top(proto p);
top(c()) main;
