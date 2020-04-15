#include <core.p4>

//architecture
control C();
package S(C c);

//user program
action f(inout bit<8> t) {
  t = t - 1;
  exit;
}

control MyC() {
  bit<8> x = 100;
  apply {
    if (f(x) == f(x)) { }
  }
}

S(MyC()) main;