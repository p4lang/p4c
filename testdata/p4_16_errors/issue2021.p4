control C();
package S(C c);
const bit<8> x = 8w1;
control MyC() {
  apply {
    .x = 8w3;
  }
}
S(MyC()) main;
