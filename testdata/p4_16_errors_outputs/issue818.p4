#include <core.p4>

parser P();
control C();
package S(P p);
extern BoolReg {
    BoolReg();
    bool get();
    void flip();
}

extern WrapControl {
    WrapControl(C c);
    void execute();
}

BoolReg() r;

parser Loop()(WrapControl c1, WrapControl c2) {
    state start {
        r.flip();
        transition select(r.get()) {
            true: q1;
            false: q2;
            default: accept;
        }
    }
    state q1 {
        c1.execute();
        transition start;
    }
    state q2 {
        c2.execute();
        transition start;
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

S(Loop(WrapControl(MyC1()), WrapControl(MyC2()))) main;

