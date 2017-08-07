#include <core.p4>

// Architecture
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

// User Program
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
    // Code for Control 1
  }
}
control MyC2() {
  apply {
    // Code for Control 2
  }
}

S(Loop(WrapControl(MyC1()), WrapControl(MyC2()))) main;
