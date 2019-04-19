#include <core.p4>

control C();
package S(C c);
extern BoolRegister {
    BoolRegister();
    bool get();
    void flip();
}

BoolRegister() r;

action a(in C c1, in C c2) {
    r.flip();
    if (r.get()) {
        c1.apply();
    }
    else {
        c2.apply();
    }
}
control MyC1() {
    apply {
    }
}

control MyC2() {
    apply {
    }
}

control MyC3() {
    apply {
        a(MyC1(), MyC2());
    }
}

S(MyC3()) main;

