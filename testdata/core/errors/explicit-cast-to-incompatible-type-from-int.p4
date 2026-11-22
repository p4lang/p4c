// generated from type-spec-nested.p4

void test<T>(in T val) {}
control c()() {
  apply {
    test((varbit<4>) 0);
  }
}
