/*
 * To compile example run (from ebpf/runtime):
 * p4c-ebpf ../externs/checksum.p4 -o test.c
 * clang -O2 -include ../externs/checksum.c -I./  -target bpf -c test.c -o test.o
 */

#include <core.p4>
#include <ebpf_model.p4>

typedef bit<48> EthernetAddress;
typedef bit<32>     IPv4Address;

// IPv4 header without options
header IPv4_h {
    bit<4>       version;
    bit<4>       ihl;
    bit<8>       diffserv;
    bit<16>      totalLen;
    bit<16>      identification;
    bit<3>       flags;
    bit<13>      fragOffset;
    bit<8>       ttl;
    bit<8>       protocol;
    bit<16>      hdrChecksum;
    IPv4Address  srcAddr;
    IPv4Address  dstAddr;
}

/** Declaration of the C extern function. */
extern bool verify_ipv4_checksum(in IPv4_h iphdr);

// standard Ethernet header
header Ethernet_h
{
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16> etherType;
}

struct Headers_t {
    Ethernet_h ethernet;
    IPv4_h     ipv4;
}

parser prs(packet_in p, out Headers_t headers) {
    state start {
        p.extract(headers.ethernet);
        p.extract(headers.ipv4);
        transition accept;
    }
}

control pipe(inout Headers_t headers, out bool pass) {

    apply {
        // verify checksum using C extern
        bool verified = verify_ipv4_checksum(headers.ipv4);
        pass = verified;
    }
}

ebpfFilter(prs(), pipe()) main;
