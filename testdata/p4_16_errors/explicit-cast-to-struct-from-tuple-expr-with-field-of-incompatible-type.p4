// generated from type-spec-nested.p4

void test<T>(in T val) {}
struct S2<T> {
  T x;
  T y;
}
struct S1<T2> {
  S2<T2> y;
}
control c()() {
  apply {
    test((S1<int<6>>) { y = { false, ... } });
  }
}
