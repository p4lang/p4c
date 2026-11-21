// generated from list2.p4

extern E<T, S> {
  E(list<tuple<T, S>> data);
}
control c0() {
  E<int<32>, bit<16>>((list<tuple<bit<32>, bit<16>>>) { { 2, 3 } }) e;
  apply {}
}
