#include <core.p4>

control c() {
    table t {
        actions = {
            @defaultonly NoAction();
        }
        const default_action = NoAction();
    }
    apply {
        t.apply();
    }
}

