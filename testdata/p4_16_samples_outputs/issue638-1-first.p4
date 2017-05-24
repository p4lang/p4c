#include <core.p4>

control c() {
    table t {
        actions = {
            @defaultonly NoAction();
        }
        default_action = NoAction();
    }
    apply {
        t.apply();
    }
}

