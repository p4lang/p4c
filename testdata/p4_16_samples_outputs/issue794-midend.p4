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
    @hidden action issue794l47() {
        rand1_1.read();
    }
    @hidden table tbl_issue794l47 {
        actions = {
            issue794l47();
        }
        const default_action = issue794l47();
    }
    apply {
        tbl_issue794l47.apply();
    }
}

Top(ingress(), caller()) main;
