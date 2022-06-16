parser MyParser1(in bit<6> ttt) {
  value_set<bit<6>>(4) myvs;
  state start {
    transition select(6w1) {
     myvs: accept;
     _: reject;
   }
 }
}
