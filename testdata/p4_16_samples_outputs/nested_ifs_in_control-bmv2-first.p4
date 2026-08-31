#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

struct addr_t {
    bit<8> f0;
    bit<8> f1;
    bit<8> f2;
    bit<8> f3;
    bit<8> f4;
    bit<8> f5;
}

header ethernet_t {
    addr_t  dstAddr;
    addr_t  srcAddr;
    bit<16> etherType;
}

struct headers {
    ethernet_t ethernet;
}

struct metadata_t {
}

parser MyParser(packet_in packet, out headers hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    state start {
        transition parse_ethernet;
    }
    state parse_ethernet {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
}

control MyVerifyChecksum(inout headers hdr, inout metadata_t meta) {
    apply {
    }
}

control MyIngress(inout headers hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    bit<8> x;
    apply {
        if (hdr.ethernet.etherType == 16w0x22f0) {
            x = 8w0;
            if (hdr.ethernet.srcAddr.f2 == 8w0xa) {
                hdr.ethernet.srcAddr.f1 = 8w0xf0;
            } else if (hdr.ethernet.srcAddr.f2 == 8w0xb) {
                if (hdr.ethernet.srcAddr.f0 == 8w0xf0) {
                    x = hdr.ethernet.srcAddr.f1;
                } else if (hdr.ethernet.srcAddr.f0 == 8w0xe0) {
                    x = hdr.ethernet.srcAddr.f1;
                }
            }
            hdr.ethernet.etherType[7:0] = x;
        }
        standard_metadata.egress_spec = 9w1;
    }
}

control MyEgress(inout headers hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control MyComputeChecksum(inout headers hdr, inout metadata_t meta) {
    apply {
    }
}

control MyDeparser(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
    }
}

V1Switch<headers, metadata_t>(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;
