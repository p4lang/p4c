#include <core.p4>
#include <ubpf_model.p4>

typedef bit<48> EthernetAddress;
typedef bit<32> IPv4Address;
header IPv4_h {
    bit<4>      version;
    bit<4>      ihl;
    bit<8>      diffserv;
    bit<16>     totalLen;
    bit<16>     identification;
    bit<3>      flags;
    bit<13>     fragOffset;
    bit<8>      ttl;
    bit<8>      protocol;
    bit<16>     hdrChecksum;
    IPv4Address srcAddr;
    IPv4Address dstAddr;
}

header Ethernet_h {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

struct Headers_t {
    Ethernet_h ethernet;
    IPv4_h     ipv4;
}

struct metadata {
}

extern bit<16> incremental_checksum(in bit<16> csum, in bit<32> old, in bit<32> new);
extern bool verify_ipv4_checksum(in IPv4_h iphdr);
parser prs(packet_in p, out Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {
    state start {
        p.extract<Ethernet_h>(headers.ethernet);
        p.extract<IPv4_h>(headers.ipv4);
        transition accept;
    }
}

control pipe(inout Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {
    @name("pipe.verified") bool verified_0;
    @name("pipe.old_addr") bit<32> old_addr_0;
    @name("pipe.new_addr") bit<32> new_addr_0;
    apply {
        verified_0 = verify_ipv4_checksum(headers.ipv4);
        if (verified_0) {
            old_addr_0 = headers.ipv4.dstAddr;
            new_addr_0 = 32w0x1020304;
            headers.ipv4.dstAddr = new_addr_0;
            headers.ipv4.hdrChecksum = incremental_checksum(headers.ipv4.hdrChecksum, old_addr_0, headers.ipv4.dstAddr);
        } else {
            mark_to_drop();
        }
    }
}

control dprs(packet_out packet, in Headers_t headers) {
    apply {
        packet.emit<Ethernet_h>(headers.ethernet);
        packet.emit<IPv4_h>(headers.ipv4);
    }
}

ubpf<Headers_t, metadata>(prs(), pipe(), dprs()) main;
