#include <core.p4>

control c() {
    action a() {}
    table t {
        key = {}
        actions = {
            a;
        }
        const entries = {}
    }

    apply {
    }
}
