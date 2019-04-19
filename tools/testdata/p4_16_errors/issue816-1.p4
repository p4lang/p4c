#include <core.p4>

// Architecture
control C();
package S(C c);

// User Program
package P(C c1, C c2);

control Dummy() {
  apply { }
}

control MyC1()(P p) {
  apply {
  }
}
control MyC2()(P p) {
  apply {
  }
}
control MyC3() {
  Dummy() d;
  P(d,d) p;
  C c1 = MyC1(p);
  C c2 = MyC2(p);
  apply {
    c1.apply();
  }
}

S(MyC3()) main;
