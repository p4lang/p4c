#include <core.p4>

control c();
parser p();
package Top(c i, p prs);
extern Random2 {
    Random2();
    bit<10> read();
}

parser caller() {
    @name("caller.rand1") Random2() rand1;
    state start {
        rand1.read();
        transition accept;
    }
}

control ingress() {
    @name("ingress.rand1") Random2() rand1_2;
    @hidden action act() {
        rand1_2.read();
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

