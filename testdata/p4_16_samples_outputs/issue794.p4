#include <core.p4>

control c();
parser p();
package Top(c i, p prs);
extern Random2 {
    Random2();
    bit<10> read();
}

parser callee(Random2 rand) {
    state start {
        rand.read();
        transition accept;
    }
}

parser caller() {
    Random2() rand1;
    callee() ca;
    state start {
        ca.apply(rand1);
        transition accept;
    }
}

control foo2(Random2 rand) {
    apply {
        rand.read();
    }
}

control ingress() {
    Random2() rand1;
    foo2() foo2_inst;
    apply {
        foo2_inst.apply(rand1);
    }
}

Top(ingress(), caller()) main;

