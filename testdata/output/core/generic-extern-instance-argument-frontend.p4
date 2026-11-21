#include <core.p4>

#define V1MODEL_VERSION 20180101
#include <v1model.p4>

extern void observe(register<bit<32>> r);
header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> ether_type;
}

struct headers_t {
    ethernet_t ethernet;
}

struct metadata_t {
}

parser MyParser(packet_in pkt, out headers_t hdr, inout metadata_t meta, inout standard_metadata_t sm) {
    state start {
        pkt.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
}

control MyVerifyChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control MyIngress(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t sm) {
    @name("MyIngress.reg") register<bit<32>>(32w1) reg_0;
    apply {
        observe(reg_0);
    }
}

control MyEgress(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t sm) {
    apply {
    }
}

control MyComputeChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control MyDeparser(packet_out pkt, in headers_t hdr) {
    apply {
        pkt.emit<ethernet_t>(hdr.ethernet);
    }
}

V1Switch<headers_t, metadata_t>(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;
