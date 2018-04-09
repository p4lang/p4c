#include <core.p4>

parser P();
control C();
package V1Switch(P p, C c);

parser MyP() {
  state start {
    transition accept;
  }
}

control MyC() {
  apply {
  }
}

V1Switch(MyP(), MyC()) main;
