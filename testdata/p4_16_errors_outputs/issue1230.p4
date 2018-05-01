#include <core.p4>

control c() {
    table t {
        actions = {
            NoAction;
        }
        size = true;
    }
    apply {
        t.apply();
    }
}

control empty();
package top(empty e);
top(c()) main;

