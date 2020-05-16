#include <core.p4>
#define UBPF_MODEL_VERSION 20200515
#include <ubpf_model.p4>

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

struct metadata { }

extern bit<16> compute_hash(in bit<32> srcAddr, in bit<32> dstAddr);

parser prs(packet_in p, out Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {
    state start {
        p.extract(headers.ethernet);
        p.extract(headers.ipv4);
        transition accept;
    }
}

control pipe(inout Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {

    apply {
        // only for test purpose
        headers.ipv4.hdrChecksum = compute_hash(headers.ipv4.srcAddr, headers.ipv4.dstAddr);
    }

}

control dprs(packet_out packet, in Headers_t headers) {
    apply {
        packet.emit(headers.ethernet);
        packet.emit(headers.ipv4);
    }
}

ubpf(prs(), pipe(), dprs()) main;

