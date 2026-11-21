// generated from issue2904.p4

control ingress()() {
  bit<4> y;
  table X {
    key = {
      y : TT_FWD;
    }
    actions = {}
  }
  apply {}
}
