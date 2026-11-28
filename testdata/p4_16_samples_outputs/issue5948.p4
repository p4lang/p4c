#include <core.p4>

control C() {
    action listed() {
    }
    action unlisted() {
    }
    table t {
        actions = {
            listed;
        }
        defaulp_action = unlisted();
    }
    apply {
        t.apply();
    }
}

control CT();
package Top(CT c);
Top(C()) main;
