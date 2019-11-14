#include <core.p4>

//Architecture
parser C();
package S(C p);

//User Program
struct s {
  bit<8> x;
}

parser D(in s z) {
  state start {
    transition accept;
  }
}

parser E() {
  tuple<bit<8>> a = { 0 };
  s b = { 1 };
  D() d;
  state start {
    d.apply(a);
    d.apply(b);
    transition accept;
  }
}

S(E()) main;
