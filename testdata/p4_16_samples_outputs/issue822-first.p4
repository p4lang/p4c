#include <core.p4>

control C();
package S(C c);
extern BoolRegister {
    BoolRegister();
}

extern ActionSelector {
    ActionSelector(BoolRegister reg);
}

BoolRegister() r;

control MyC1() {
    ActionSelector(r) action_selector;
    apply {
    }
}

S(MyC1()) main;

