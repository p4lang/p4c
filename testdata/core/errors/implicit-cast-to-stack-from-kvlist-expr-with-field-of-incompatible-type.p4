// generated from default-initializer.p4

header H {}
control C()() {
  H[2] stack;
  apply {
    tuple<> h0;
    stack = { h0, ... };
  }
}
