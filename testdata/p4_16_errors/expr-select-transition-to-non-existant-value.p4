// generated from issue3343.p4

parser MyParser10() {
  value_set<bit<6>>(4) myvs;
  state start {
    transition select(6w1) {
      (myvs): lazy;
    }
  }
}
