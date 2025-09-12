// generated from stack-init.p4

header H<T> {
  bit<32> b;
  T t;
}
control c0() {
  apply {
    H<bit<32>>[3] s;
    s = (H<bit<32>>[3]) { { 0, 1 }, "r" };
  }
}
