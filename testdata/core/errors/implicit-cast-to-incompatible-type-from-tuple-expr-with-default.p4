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
  table t_lpm {
    key = {
      h.h.l : lpm;
    }
    actions = {}
    const entries = {
      ({ ... }) : a_with_control_params(12);
    }
  }
  apply {}
}
