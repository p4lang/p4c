// generated from issue4883_dup_has_returned_decl.p4

struct S {}
control C(inout tuple<> S)() {
  apply {}
}
control C2()() {
  apply {
    S s;
    C.apply(s);
  }
}
