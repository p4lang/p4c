#include <core.p4>

control c();
package p(c c);
control myc() {
    action a() {
    }
    action b() {
    }
    table t {
        key = {
        }
        actions = {
            a();
            @defaultonly NoAction();
        }
        const entries = {
                        default : a();
        }
        default_action = NoAction();
    }
    apply {
        t.apply();
    }
}

p(myc()) main;

