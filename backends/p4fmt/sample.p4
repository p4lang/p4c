/* -*- P4_16 -*- */

const bit<16> TYPE_IPV4 = 0x800;

typedef bit<9>  egressSpec_t;
typedef bit<48> macAddr_t;
typedef bit<32> ip4Addr_t;

header ethernet_t {
    macAddr_t dstAddr;

    // comm1
    macAddr_t srcAddr; // comm2

    bit<16>   etherType;
}

struct headers {
    ethernet_t   ethernet;
}
