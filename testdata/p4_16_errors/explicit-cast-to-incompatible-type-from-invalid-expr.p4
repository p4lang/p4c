// generated from type-spec-nested.p4

struct S2<T> {
  T x;
  T y;
}
control c(inout bit<8> a)() {
  apply {
    a = (S2<int<6>>) { x = {#}, y = 0 };
  }
}
