#include <core.p4>

// Architecture
control C();
package S(C c);

extern WrapControls<T1,T2> {
  WrapControls(T1 t1, T2 t2);
  void execute1(); // invokes c1.apply on self
  void execute2(); // invokes c2.apply on self
}

//User Program
control MyC1(WrapControls<_,_> wc) {
  apply {
    wc.execute2();
  }
}
control MyC2(WrapControls<_,_> wc) {
  apply {
    wc.execute1();
  }
}
control MyC3() {
  MyC1() c1;
  MyC2() c2;
  WrapControls<MyC1,MyC2>(c1,c2) wc;
  apply {
    c1.apply(wc);
  }
}

S(MyC3()) main;
