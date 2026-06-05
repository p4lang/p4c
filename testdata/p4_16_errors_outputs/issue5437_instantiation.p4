#include <core.p4>

control MAPipe();
package SimpleArch(MAPipe p);
control MyMAPipe() {
    action a0() {
    }
    table t0 {
        actions = {
            a0;
        }
        default_action = a0();
    }
    apply {
        t0.apply();
    }
}

SimpleArch(MyMAPipe()()) main;
