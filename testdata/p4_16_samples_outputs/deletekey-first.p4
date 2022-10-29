#include <core.p4>

control c() {
    action a() {
    }
    table t1 {
        actions = {
            a();
            @defaultonly NoAction();
        }
        default_action = NoAction();
    }
    table t2 {
        actions = {
            a();
            @defaultonly NoAction();
        }
        default_action = NoAction();
    }
    apply {
        t1.apply();
        t2.apply();
    }
}

control C();
package top(C _c);
top(c()) main;
