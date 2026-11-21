// generated from gauntlet_hdr_int_initializer-bmv2.p4

header ethernet_t {
  bit<16> eth_type;
}
struct Headers {
  ethernet_t eth_hdr;
}
control ingress(inout Headers h)() {
  apply {
    ethernet_t tmp_hdr = { 1, 1, 1 };
    h.eth_hdr.eth_type = (tmp_hdr.eth_type |-| false);
  }
}
