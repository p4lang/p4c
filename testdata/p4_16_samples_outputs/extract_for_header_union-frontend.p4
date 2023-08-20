#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header addr_ipv4_t {
    bit<32> addr;
}

header addr_ipv6_t {
    bit<128> addr;
}

header_union addr_t1 {
    addr_ipv4_t ipv4;
    addr_ipv6_t ipv6;
}

header_union addr_t2 {
    addr_ipv4_t ipv4;
    addr_ipv6_t ipv6;
}

struct metadata {
}

struct headers {
    addr_t1 addr_src1;
    addr_t2 addr_src2;
}

parser ProtParser(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        packet.extract<addr_ipv4_t>(hdr.addr_src1.ipv4);
        hdr.addr_src1.ipv4.addr = hdr.addr_src2.ipv4.addr;
        transition reject;
    }
}

control ProtVerifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control ProtIngress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control ProtEgress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control ProtComputeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control ProtDeparser(packet_out packet, in headers hdr) {
    apply {
        packet.emit<headers>(hdr);
    }
}

V1Switch<headers, metadata>(ProtParser(), ProtVerifyChecksum(), ProtIngress(), ProtEgress(), ProtComputeChecksum(), ProtDeparser()) main;
