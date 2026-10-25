// generated from table-entries-lpm-bmv2.p4

match_kind {
  exact,
  ternary,
  lpm
}
header hdr {
  bool l;
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
  }
  apply {}
}
