// generated from list2.p4

extern E<T, S> {
  E(list<tuple<T, S>> data);
}
control c()() {
  E<list<int>, bit<16>>((list<tuple<bit<32>, bit<16>>>) { }) e;
  apply {}
}
