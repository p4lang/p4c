#include <core.p4>

control c();
parser p();
package Top(c i, p prs);
extern Random2 {
    Random2();
    bit<10> read();
}

parser caller() {
    @name("caller.rand1") Random2() rand1_0;
    state start {
        rand1_0.read();
        transition accept;
    }
}

control ingress() {
    @name("ingress.rand1") Random2() rand1_1;
    @hidden action act() {
        rand1_1.read();
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        tbl_act.apply();
    }
}

Top(ingress(), caller()) main;

