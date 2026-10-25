// generated from type-spec-nested.p4

void test<T>(in T val) {}
control c0() {
  apply {
    test((tuple<>) { x = 0, y = 0 });
  }
}
