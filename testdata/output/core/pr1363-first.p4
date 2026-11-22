#include <core.p4>

typedef bit<1> implementation;
extern ActionProfile {
    ActionProfile(bit<32> size);
}

control c() {
    table t {
        actions = {
            NoAction();
        }
        implementation = ActionProfile(32w32);
        default_action = NoAction();
    }
    apply {
    }
}

