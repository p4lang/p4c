#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

struct metadata {
}

header BITS {
    bit<8> bt;
    int<8> it;
    bool   b;
    bit<7> x;
}

struct headers {
    BITS   bits;
    bit<8> bt;
    int<8> it;
    bool   b;
    bit<7> x;
}

parser MyParser(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        packet.extract<BITS>(hdr.bits);
        hdr.bt = hdr.bits.bt;
        hdr.it = hdr.bits.it;
        hdr.b = hdr.bits.b;
        hdr.x = hdr.bits.x;
        transition accept;
    }
}

control mauif(inout headers hdr, inout metadata meta, inout standard_metadata_t sm) {
    @hidden action struct_with_stbmv2l43() {
        hdr.x = 7w1;
        hdr.bt = 8w1;
        hdr.it = 8s1;
    }
    @hidden action struct_with_stbmv2l47() {
        hdr.bt = 8w7;
        hdr.it = -8s1;
        hdr.x = 7w0;
    }
    @hidden table tbl_struct_with_stbmv2l43 {
        actions = {
            struct_with_stbmv2l43();
        }
        const default_action = struct_with_stbmv2l43();
    }
    @hidden table tbl_struct_with_stbmv2l47 {
        actions = {
            struct_with_stbmv2l47();
        }
        const default_action = struct_with_stbmv2l47();
    }
    apply {
        if (hdr.b) {
            tbl_struct_with_stbmv2l43.apply();
        } else {
            tbl_struct_with_stbmv2l47.apply();
        }
    }
}

control mau(inout headers hdr, inout metadata meta, inout standard_metadata_t sm) {
    apply {
    }
}

control deparse(packet_out pkt, in headers hdr) {
    apply {
    }
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch<headers, metadata>(MyParser(), verifyChecksum(), mauif(), mau(), computeChecksum(), deparse()) main;

