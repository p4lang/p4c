#include <core.p4>

control c(out bool x) {
    bool tmp;
    bool tmp_0;
    bool tmp_1;
    @name("t1") table t1_0 {
        key = {
            x: exact @name("x") ;
        }
        actions = {
            NoAction();
        }
        default_action = NoAction();
    }
    @name("t2") table t2_0 {
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
        tmp = t1_0.apply().hit;
        if (!tmp) 
            tmp_0 = false;
        else {
            tmp_1 = t2_0.apply().hit;
            tmp_0 = tmp_1;
        }
        if (tmp_0) 
            x = false;
    }
}

control proto(out bool x);
package top(proto p);
top(c()) main;

