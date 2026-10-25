// generated from table-entries-lpm-bmv2.p4

match_kind {
  exact,
  ternary,
  lpm
}
header hdr {
  bit<8> l;
}
struct Header_t {
  hdr h;
}
control ingress(inout Header_t h)() {
  action a_with_control_params(bit<9> x) {}
  table t_lpm {
    key = {
      h.h.l : lpm;
    }
    actions = {
      a_with_control_params;
    }
    const entries = {
      priority = (false % true) : 18 : a_with_control_params(12);
    }
  }
  apply {}
}
