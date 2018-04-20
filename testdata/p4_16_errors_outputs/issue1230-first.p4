#include <core.p4>

control c() {
    table t {
        actions = {
            NoAction();
        }
        size = true;
        default_action = NoAction();
    }
    apply {
        t.apply();
    }
}

control empty();
package top(empty e);
top(c()) main;

