#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

typedef bit<48> mac_addr_t;
header ethernet_t {
    mac_addr_t dstAddr;
    mac_addr_t srcAddr;
    bit<16>    etherType;
}

header ml_hdr_t {
    bit<8> idx1;
    bit<8> idx2;
}

header vec_e_t {
    bit<8> e;
}

struct headers {
    ethernet_t ethernet;
    ml_hdr_t   ml;
    vec_e_t[8] vector;
}

struct metadata_t {
}

parser MyParser(packet_in packet, out headers hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        packet.extract<ml_hdr_t>(hdr.ml);
        packet.extract<vec_e_t>(hdr.vector[0]);
        packet.extract<vec_e_t>(hdr.vector[1]);
        packet.extract<vec_e_t>(hdr.vector[2]);
        packet.extract<vec_e_t>(hdr.vector[3]);
        packet.extract<vec_e_t>(hdr.vector[4]);
        packet.extract<vec_e_t>(hdr.vector[5]);
        packet.extract<vec_e_t>(hdr.vector[6]);
        packet.extract<vec_e_t>(hdr.vector[7]);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    apply {
        hdr.vector[hdr.ml.idx1 - (hdr.ml.idx2 >> 8w1)].e = hdr.ethernet.etherType[15:8] + 8w7;
        hdr.ethernet.etherType[7:0] = hdr.vector[(hdr.ml.idx2 ^ 8w0x7) & 8w0x7].e;
        hdr.vector[hdr.vector[hdr.ethernet.dstAddr[39:32] & 8w0x7].e & 8w0x7].e = hdr.ethernet.dstAddr[47:40];
    }
}

control egress(inout headers hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control MyVerifyChecksum(inout headers hdr, inout metadata_t meta) {
    apply {
    }
}

control MyComputeChecksum(inout headers hdr, inout metadata_t meta) {
    apply {
    }
}

control MyDeparser(packet_out packet, in headers hdr) {
    apply {
        packet.emit<headers>(hdr);
    }
}

V1Switch<headers, metadata_t>(MyParser(), MyVerifyChecksum(), ingress(), egress(), MyComputeChecksum(), MyDeparser()) main;
