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
    @name("MyC1.action_selector") ActionSelector(r) action_selector_0;
    apply {
    }
}

S(MyC1()) main;

