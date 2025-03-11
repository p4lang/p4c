#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header standard_t<T> {
    T src;
    T dst;
}

header standard_t_bit8 {
    bit<8> src;
    bit<8> dst;
}

struct headers_t<T> {
    standard_t<T> standard;
}

struct headers_t_bit8 {
    standard_t_bit8 standard;
}

struct meta_t {
}

parser MyParser(packet_in pkt, out headers_t_bit8 hdr, inout meta_t meta, inout standard_metadata_t std_meta) {
    state start {
        pkt.extract<standard_t_bit8>(hdr.standard);
        transition accept;
    }
}

control MyVerifyChecksum(inout headers_t_bit8 hdr, inout meta_t meta) {
    apply {
    }
}

control MyComputeChecksum(inout headers_t_bit8 hdr, inout meta_t meta) {
    apply {
    }
}

control MyIngress(inout headers_t_bit8 hdr, inout meta_t meta, inout standard_metadata_t std_meta) {
    apply {
    }
}

control MyEgress(inout headers_t_bit8 hdr, inout meta_t meta, inout standard_metadata_t std_meta) {
    apply {
    }
}

control MyDeparser(packet_out pkt, in headers_t_bit8 hdr) {
    apply {
    }
}

V1Switch<headers_t_bit8, meta_t>(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;
