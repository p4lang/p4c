// generated from table-entries-lpm-bmv2.p4

header hdr {
  varbit<8> l;
}
struct Header_t {
  hdr h;
}
control ingress(inout Header_t h)() {
  table t {
    key = {
      h.h.l : lpm;
    }
    actions = {}
  }
  apply {}
}
