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
    @name("rand1") Random2() rand1_0;
    @name("ca") callee() ca_0;
    state start {
        ca_0.apply(rand1_0);
        transition accept;
    }
}

control foo2(Random2 rand) {
    apply {
        rand.read();
    }
}

control ingress() {
    @name("rand1") Random2() rand1_1;
    @name("foo2_inst") foo2() foo2_inst_0;
    apply {
        foo2_inst_0.apply(rand1_1);
    }
}

Top(ingress(), caller()) main;

