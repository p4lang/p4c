// generated from issue2335-1.p4

control c(inout bit<8> val)(int a) {
  apply {}
}
control ingress(inout bit<8> a)() {
  c(0) x;
  apply {
    x.apply();
  }
}
