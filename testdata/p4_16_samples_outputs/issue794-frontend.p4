#include <core.p4>

control c();
parser p();
package Top(c i, p prs);
extern Random2 {
    Random2();
    bit<10> read();
}

parser caller() {
    @name("rand1") Random2() rand1;
    state start {
        transition callee_start;
    }
    state callee_start {
        rand1.read();
        transition start_0;
    }
    state start_0 {
        transition accept;
    }
}

control ingress() {
    @name("rand1") Random2() rand1_2;
    apply {
        rand1_2.read();
    }
}

Top(ingress(), caller()) main;

