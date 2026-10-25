// generated from default-initializer.p4

header H {
  varbit<32> b;
}
control C()() {
  apply {
    H h = (H) { b = 2, ... };
  }
}
