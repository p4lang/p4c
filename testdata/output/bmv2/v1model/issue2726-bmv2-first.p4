#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

typedef bit<48> mac_addr_t;
header aggregator_t {
    bit<8> base0;
    bit<8> base1;
    bit<8> base2;
    bit<8> base3;
    bit<8> val;
}

header vec_e_t {
    bit<8> e;
}

header ml_hdr_t {
    int<8> idx;
}

header ethernet_t {
    mac_addr_t dstAddr;
    mac_addr_t srcAddr;
    bit<16>    etherType;
}

struct headers {
    ethernet_t      ethernet;
    ml_hdr_t        ml;
    vec_e_t[3]      vector;
    aggregator_t[3] pool;
}

struct metadata_t {
    int<8> counter;
}

parser MyParser(packet_in packet, out headers hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        packet.extract<ml_hdr_t>(hdr.ml);
        packet.extract<vec_e_t>(hdr.vector[0]);
        packet.extract<vec_e_t>(hdr.vector[1]);
        packet.extract<vec_e_t>(hdr.vector[2]);
        packet.extract<aggregator_t>(hdr.pool[0]);
        packet.extract<aggregator_t>(hdr.pool[1]);
        packet.extract<aggregator_t>(hdr.pool[2]);
        meta.counter = 8s0;
        transition accept;
    }
}

enum int<8> Index_t {
    ZERO = 8s0,
    ONE = 8s1,
    TWO = 8s2
}

control ingress(inout headers hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    apply {
        meta.counter = meta.counter + 8s1;
        hdr.vector[0].e = hdr.pool[1].val + 8w1;
        Index_t i = (Index_t)hdr.ml.idx;
        hdr.pool[i].val = hdr.vector[0].e;
        hdr.pool[i].base2 = hdr.vector[0].e;
        hdr.vector[1].e = hdr.pool[i].base0;
        hdr.pool[i].base0 = hdr.pool[i].base1 + 8w1;
        standard_metadata.egress_spec = standard_metadata.ingress_port;
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
