#include <core.p4>

control C() {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("C.listed") action listed() {
    }
    @name("C.unlisted") action unlisted() {
    }
    @name("C.t") table t_0 {
        actions = {
            listed();
            @defaultonly NoAction_1();
        }
        defaulp_action = unlisted();
        default_action = NoAction_1();
    }
    apply {
        t_0.apply();
    }
}

control CT();
package Top(CT c);
Top(C()) main;
