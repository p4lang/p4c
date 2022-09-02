#include <core.p4>

control c() {
    action a() { }

    table t1 {
        key = {}
        actions = {a;}
    }

    table t2 {
        actions = {a;}
    }

    apply {
        t1.apply();
        t2.apply();
    }
}

control C();
package top(C _c);

top(c()) main;
