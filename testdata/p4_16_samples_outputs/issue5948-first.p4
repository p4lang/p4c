#include <core.p4>

control C() {
    action listed() {
    }
    action unlisted() {
    }
    table t {
        actions = {
            listed();
            @defaultonly NoAction();
        }
        defaulp_action = unlisted();
        default_action = NoAction();
    }
    apply {
        t.apply();
    }
}

control CT();
package Top(CT c);
Top(C()) main;
