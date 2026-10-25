// generated from issue1830.p4

void foo<T>(in T x) {}
void bar() {
  foo<string>(true);
}
