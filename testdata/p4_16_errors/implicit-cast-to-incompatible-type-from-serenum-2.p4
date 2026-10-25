// generated from issue3623-1.p4

enum bit<4> e { a = 1 };
control c(in error v)() {
  apply {
    switch (v) {
      e.a:
      default: {}
    }
  }
}
