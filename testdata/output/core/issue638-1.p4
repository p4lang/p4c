#include <core.p4>

control c() {
    table t {
        actions = {
        }
    }
    apply {
        t.apply();
    }
}

