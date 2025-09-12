// generated from table-entries-range-bmv2.p4

match_kind {
  range,
  optional,
  selector
}
header hdr {
  bit<8> r;
}
struct Header_t {
  hdr h;
}
control ingress(inout Header_t h)() {
  action a_with_control_params(bit<9> x) {}
  table t_range {
    key = {
      h.h.r : range;
    }
    actions = {}
    const entries = {
      ("x" .. ...) : a_with_control_params(22);
    }
  }
  apply {}
}
