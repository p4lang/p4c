// generated from issue3671-1.p4

struct s {
  bit<1> f0;
  bit<1> f1;
}
extern bit<1> ext();
control c()() {
  action a1(in s v) {}
  table t {
    actions = {
      a1((s) { ext(), ext() });
    }
    default_action = a1((match_kind) (...));
  }
  apply {}
}
