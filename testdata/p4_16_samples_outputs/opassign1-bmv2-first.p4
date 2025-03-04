#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header data_t {
    bit<32> f1;
    bit<32> f2;
    bit<16> h1;
    bit<16> h2;
    bit<8>  b1;
    bit<8>  b2;
    bit<8>  b3;
    bit<8>  b4;
}

struct metadata {
}

struct headers {
    data_t data;
}

parser p(packet_in b, out headers hdr, inout metadata meta, inout standard_metadata_t stdmeta) {
    state start {
        b.extract<data_t>(hdr.data);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t stdmeta) {
    apply {
        hdr.data.h1 += hdr.data.h2;
        hdr.data.b1 -= hdr.data.b2;
        hdr.data.b3 ^= hdr.data.b4;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t stdmeta) {
    apply {
    }
}

control vc(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control uc(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control deparser(packet_out packet, in headers hdr) {
    apply {
        packet.emit<headers>(hdr);
    }
}

V1Switch<headers, metadata>(p(), vc(), ingress(), egress(), uc(), deparser()) main;
