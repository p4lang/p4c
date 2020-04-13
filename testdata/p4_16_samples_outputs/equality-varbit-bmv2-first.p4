#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header H {
    bit<8>     s;
    varbit<32> v;
}

struct metadata {
}

struct headers {
    H h;
}

parser p(packet_in b, out headers hdr, inout metadata meta, inout standard_metadata_t stdmeta) {
    state start {
        b.extract<H>(hdr.h, 32w32);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t stdmeta) {
    apply {
        stdmeta.egress_spec = 9w0;
        H h = hdr.h;
        if (hdr.h.v == h.v) {
            stdmeta.egress_spec = 9w1;
        }
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

