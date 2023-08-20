#include <core.p4>

control c() {
    action a() {
    }
    table t {
        actions = {
            a();
            @defaultonly NoAction();
        }
        const entries = {
        }
        default_action = NoAction();
    }
    apply {
    }
}

