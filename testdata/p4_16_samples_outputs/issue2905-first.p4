#include <core.p4>

control c() {
    action a() {
    }
    table t {
        key = {
        }
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

