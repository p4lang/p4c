parser MyParser2(in bool t) {
  state start {
    transition select(t) {
     1w1: accept;
     _: reject;
   }
 }
}
