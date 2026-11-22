/*
 * To compile example run (from ebpf/runtime):
 * p4c-ebpf ../externs/stateful-firewall.p4 -o test.c
 * clang -O2 -include ../externs/conntrack.c -I./  -target bpf -c test.c -o test.o
 */

#include <core.p4>
#include <ebpf_model.p4>

typedef bit<48> EthernetAddress;
typedef bit<32>     IPv4Address;

header Ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

header Ipv4_t {
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

header Tcp_t {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<32> seqNo;
    bit<32> ackNo;
    bit<4>  dataOffset;
    bit<3>  res;
    bit<3>  ecn;
    bit<1> urgent;
    bit<1> ack;
    bit<1> psh;
    bit<1> rst;
    bit<1> syn;
    bit<1> fin;
    bit<16> window;
    bit<16> checksum;
    bit<16> urgentPtr;
}

struct Headers_t {
    Ethernet_t       ethernet;
    Ipv4_t           ipv4;
    Tcp_t            tcp;
}

/** Declaration of the C extern function. */
extern bool tcp_conntrack(in Headers_t hdrs);

parser prs(packet_in p, out Headers_t headers) {
    state start {
        p.extract(headers.ethernet);
        p.extract(headers.ipv4);
        p.extract(headers.tcp);
        transition accept;
    }
}

control pipe(inout Headers_t headers, out bool pass) {
    apply {
        pass = false;
        if (headers.tcp.isValid()) {
            bool allow = tcp_conntrack(headers);
            if (allow)
                pass = true;
        }
    }
}

ebpfFilter(prs(), pipe()) main;

