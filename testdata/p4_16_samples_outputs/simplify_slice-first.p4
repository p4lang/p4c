#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

typedef bit<48> EthernetAddress;
typedef bit<32> IPv4Address;
header Ethernet_h {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

header Ipv4_h {
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

struct Headers {
    Ethernet_h ethernet;
    Ipv4_h     ip;
}

struct Metadata {
}

parser P(packet_in b, out Headers p, inout Metadata meta, inout standard_metadata_t standard_meta) {
    state start {
        transition accept;
    }
}

control Ing(inout Headers headers, inout Metadata meta, inout standard_metadata_t standard_meta) {
    register<bit<8>>(32w2) debug;
    apply {
        bit<8> n = 8w0b11111111;
        bit<8> m = 8w0b11111111;
        bit<8> x = 8w0b11111111;
        n[7:4] = 4w0;
        m[7:5] = 3w0;
        x[5:5] = 1w0;
        debug.write(32w1, n);
        standard_meta.egress_spec = 9w0;
    }
}

control Eg(inout Headers hdrs, inout Metadata meta, inout standard_metadata_t standard_meta) {
    apply {
    }
}

control DP(packet_out b, in Headers p) {
    apply {
        b.emit<Ethernet_h>(p.ethernet);
        b.emit<Ipv4_h>(p.ip);
    }
}

control Verify(inout Headers hdrs, inout Metadata meta) {
    apply {
    }
}

control Compute(inout Headers hdr, inout Metadata meta) {
    apply {
    }
}

V1Switch<Headers, Metadata>(P(), Verify(), Ing(), Eg(), Compute(), DP()) main;

