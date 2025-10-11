// generated from issue1739-bmv2.p4

header H {}
control ingress(inout H hdr)() {
  table ipv4_da_lpm {
    key = {
      hdr.dstAddr : lpm;
    }
    actions = {}
  }
  apply {}
}
