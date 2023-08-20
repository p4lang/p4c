#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header ipv4_t {
    bit<4>  version;
    bit<4>  ihl;
    bit<8>  diffserv;
    bit<16> packet_length;
    bit<16> identification;
    bit<3>  flags;
    bit<13> fragOffset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> hdrChecksum;
    bit<32> srcAddr;
    bit<32> dstAddr;
}

struct headers {
    ethernet_t ethernet;
    ipv4_t     ipv4;
}

struct metadata {
}

parser MyParser(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        transition accept;
    }
}

control MyIngress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("MyIngress.drop") action drop() {
        mark_to_drop(standard_metadata);
    }
    @name("MyIngress.actTbl") action actTbl(@name("id") bit<24> id, @name("ip") bit<32> ip) {
    }
    @name("MyIngress.ingress_tbl") table ingress_tbl_0 {
        key = {
            hdr.ipv4.dstAddr: exact @name("hdr.ipv4.dstAddr");
        }
        actions = {
            actTbl();
            drop();
        }
        const default_action = drop();
        const entries = {
                        32w0x20020420 : actTbl(24w42, 32w0x20024200);
        }
    }
    apply {
        if (hdr.ipv4.isValid()) {
            ingress_tbl_0.apply();
        }
    }
}

control MyEgress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control MyDeparser(packet_out packet, in headers hdr) {
    apply {
    }
}

control MyVerifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control MyComputeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch<headers, metadata>(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;
