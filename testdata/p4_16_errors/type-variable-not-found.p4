// generated from list9.p4

struct S<T> {}
extern E {
  E(list<S<bit<32>>> data);
}
control c()() {
  E((list<c<bit<32>>>) {}) e;
  apply {}
}
