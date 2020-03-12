#include <core.p4>
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

/** Declaration of the C extern function. */
extern bit<16> incremental_checksum(in bit<16> csum, in bit<32> old, in bit<32> new);
extern bool verify_ipv4_checksum(in IPv4_h iphdr);

parser prs(packet_in p, out Headers_t headers, inout metadata meta) {
    state start {
        p.extract(headers.ethernet);
        p.extract(headers.ipv4);
        transition accept;
    }
}

control pipe(inout Headers_t headers, inout metadata meta) {

    apply {
        bool verified = verify_ipv4_checksum(headers.ipv4);
        if (verified == true) {
            bit<32> old_addr = headers.ipv4.dstAddr;
            bit<32> new_addr = 32w0x01020304;
            headers.ipv4.dstAddr = new_addr;
            headers.ipv4.hdrChecksum = incremental_checksum(headers.ipv4.hdrChecksum, old_addr, headers.ipv4.dstAddr);
        } else {
            mark_to_drop();
        }
    }

}

control dprs(packet_out packet, in Headers_t headers) {
    apply {
        packet.emit(headers.ethernet);
        packet.emit(headers.ipv4);
    }
}

ubpf(prs(), pipe(), dprs()) main;

