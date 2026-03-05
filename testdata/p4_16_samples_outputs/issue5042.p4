#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header h_t {
    bit<8> f;
}

struct metadata_t {
    bit<8> x;
}

struct headers_t {
    h_t h;
}

parser p(packet_in pkt, out headers_t hdr, inout metadata_t meta, inout standard_metadata_t std_meta) {
    state start {
        pkt.extract(hdr.h);
        meta.x = hdr.h.f;
        transition accept;
    }
}

control ingress(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t std_meta) {
    action a(bit<8> data) {
        hdr.h.f = data;
    }
    table t {
        key = {
            hdr.h.f: exact;
        }
        actions = {
            a;
        }
    }
    apply {
        a(meta.x);
        t.apply();
    }
}

control egress(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t std_meta) {
    apply {
    }
}

control deparser(packet_out pkt, in headers_t hdr) {
    apply {
        pkt.emit(hdr.h);
    }
}

control verifyChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control computeChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

V1Switch(p(), verifyChecksum(), ingress(), egress(), computeChecksum(), deparser()) main;
