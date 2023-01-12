header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}
struct Headers {
    ethernet_t eth_hdr;
}

void f (inout Headers h) {
  h.eth_hdr = h.eth_hdr.eth_type == 1 ? {1, 1, 1} : {2, 2, 2};
}