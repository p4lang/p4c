// generated from entries-prio.p4

match_kind {
  exact,
  ternary,
  lpm
}
header H {
  bit<32> e;
  bit<32> t;
}
struct Headers {
  H h;
}
control c(in Headers h)() {
  action a_params(bit<32> param) {}
  table t_exact_ternary {
    key = {
      h.h.t : ternary;
      h.h.e : exact;
    }
    actions = {}
    entries = {
      (6, _) : a_params(6);
    }
  }
  apply {}
}
