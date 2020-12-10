#ifndef _VENDOR_P4_
#define _VENDOR_P4_

#include <core.p4>

enum Choice {
    First,
    Second
}

enum bit<8> Rate {
    Slow = 1,
    Medium = 2
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header ipv4_t {
    bit<4>  version;
    bit<4>  ihl;
    bit<8>  diffserv;
    bit<16> totalLen;
    bit<16> identification;
    bit<3>  flags;
    bit<13> fragOffset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> hdrChecksum;
    bit<32> srcAddr;
    bit<32> dstAddr;
}

header udp_t {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<16> hdr_length;
    bit<16> checksum;
}

struct meta_t {
}

struct headers_t {
    ethernet_t eth;
    ipv4_t     ipv4;
}

#endif
