// generated from default-initializer.p4

header H {}
control C()() {
  H[2] stack;
  apply {
    stack = (list<match_kind>) { (H) {#}, ... };
  }
}
