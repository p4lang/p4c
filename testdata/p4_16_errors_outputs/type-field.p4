#include <core.p4>
#include <v1model.p4>

header d_t {
    bit<32> f;
}

struct h {
    d_t d;
}

struct m {
}

parser MyParser(packet_in b, out h hdr, inout m meta, inout standard_metadata_t standard_metadata) {
    state start {
        b.extract(hdr.d);
        transition accept;
    }
}

control MyVerifyChecksum(inout h hdr, inout m meta) {
    apply {
    }
}

control MyIngress(inout h hdr, inout m meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control MyEgress(inout h hdr, inout m meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control MyComputeChecksum(inout h hdr, inout m meta) {
    apply {
    }
}

control MyDeparser(packet_out b, in h hdr) {
    apply {
        b.emit(h.d);
    }
}

V1Switch(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;

