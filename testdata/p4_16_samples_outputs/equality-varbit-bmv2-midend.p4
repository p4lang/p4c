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
    @name("ingress.h") H h_0;
    @hidden action equalityvarbitbmv2l33() {
        stdmeta.egress_spec = 9w1;
    }
    @hidden action equalityvarbitbmv2l29() {
        stdmeta.egress_spec = 9w0;
        h_0 = hdr.h;
    }
    @hidden table tbl_equalityvarbitbmv2l29 {
        actions = {
            equalityvarbitbmv2l29();
        }
        const default_action = equalityvarbitbmv2l29();
    }
    @hidden table tbl_equalityvarbitbmv2l33 {
        actions = {
            equalityvarbitbmv2l33();
        }
        const default_action = equalityvarbitbmv2l33();
    }
    apply {
        tbl_equalityvarbitbmv2l29.apply();
        if (hdr.h.v == h_0.v) {
            tbl_equalityvarbitbmv2l33.apply();
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
        packet.emit<H>(hdr.h);
    }
}

V1Switch<headers, metadata>(p(), vc(), ingress(), egress(), uc(), deparser()) main;
