#include <core.p4>

control c(out bool x) {
    @name(".NoAction") action NoAction_0() {
    }
    @name(".NoAction") action NoAction_3() {
    }
    bool tmp_2;
    bool tmp_3;
    bool tmp_4;
    @name("c.t1") table t1 {
        key = {
            x: exact @name("x") ;
        }
        actions = {
            NoAction_0();
        }
        default_action = NoAction_0();
    }
    @name("c.t2") table t2 {
        key = {
            x: exact @name("x") ;
        }
        actions = {
            NoAction_3();
        }
        default_action = NoAction_3();
    }
    apply {
        x = true;
        tmp_2 = t1.apply().hit;
        if (!tmp_2) 
            tmp_3 = false;
        else {
            tmp_4 = t2.apply().hit;
            tmp_3 = tmp_4;
        }
        if (tmp_3) 
            x = false;
    }
}

control proto(out bool x);
package top(proto p);
top(c()) main;

