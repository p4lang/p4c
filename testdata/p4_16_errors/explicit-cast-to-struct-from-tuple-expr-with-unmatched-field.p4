// generated from default-initializer.p4

struct S {}
control C()() {
  apply {
    S s = (S) { b = 2, ... };
  }
}
