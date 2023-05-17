control C();
package P(C a);

control MyC(@optional inout bool b) {
  apply {
     b = !b;
  }
}

control MyD() {
  MyC() c;
  apply {
    c.apply();
  }
}

P(MyD()) main;
